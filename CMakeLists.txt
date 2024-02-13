cmake_minimum_required(VERSION 3.18)

project(libedit)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#add_definitions(-DDEBUG_TTY -DDEBUG_KEY -DDEBUG_SCREEN -DDEBUG_PASTE -DDEBUG_EDIT)

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

#
# Find Windows SDK includes directory and create crtheaders.h
#
if(WIN32)
    if(CMAKE_CL_64)
        set(CMAKE_MSVC_ARCH x64)
    else(CMAKE_CL_64)
        set(CMAKE_MSVC_ARCH x86)
    endif(CMAKE_CL_64)

    # Find the Windows Kits directory
    # code extracted from InstallRequiredSystemLibraries.cmake
    get_filename_component(windows_kits_dir
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE)
    set(programfilesx86 "ProgramFiles(x86)")
    if(";${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION};$ENV{UCRTVersion};$ENV{WindowsSDKVersion};" MATCHES [=[;(10\.[0-9.]+)[;\]]=])
        set(winkit_ucrt_version "${CMAKE_MATCH_1}")
    else()
        set(winkit_ucrt_version "")
    endif()
    find_path(WINDOWS_KITS_DIR
        NAMES
            Redist/${winkit_ucrt_version}/ucrt/DLLs/${CMAKE_MSVC_ARCH}/ucrtbase.dll
            Redist/ucrt/DLLs/${CMAKE_MSVC_ARCH}/ucrtbase.dll
        PATHS
            $ENV{CMAKE_WINDOWS_KITS_10_DIR}
            "${windows_kits_dir}"
            "$ENV{ProgramFiles}/Windows Kits/10"
            "$ENV{${programfilesx86}}/Windows Kits/10"
    )

    # Find ucrt includes directory
    string(COMPARE NOTEQUAL "${winkit_ucrt_version}" "" __no_ucrt_version)
    if (__no_ucrt_version)
        file(GLOB winkit_versions LIST_DIRECTORIES true RELATIVE
            ${WINDOWS_KITS_DIR}/Include/ ${WINDOWS_KITS_DIR}/Include/*)
        list(SORT winkit_versions COMPARE NATURAL ORDER DESCENDING)
        list(GET winkit_versions 0 winkit_ucrt_version)
    endif()
    set(winkit_ucrt_includes_dir
        ${WINDOWS_KITS_DIR}/Include/${winkit_ucrt_version}/ucrt)

    # Print summary of Windows Kit information
    message("Windows Kit Directory: ${WINDOWS_KITS_DIR}")
    message("Windows Kit Version: ${winkit_ucrt_version}")
    message("Windows Kit UCRT Includes: ${winkit_ucrt_includes_dir}")

    # Create crtheaders.h
    set(crtheaders_h "${PROJECT_BINARY_DIR}/include/crtheaders.h")
    file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/include")
    file(WRITE ${crtheaders_h} "#pragma once\n")
    file(GLOB_RECURSE winkit_ucrt_includes RELATIVE ${winkit_ucrt_includes_dir}
        ${winkit_ucrt_includes_dir}/*.h)
    foreach(winkit_ucrt_include ${winkit_ucrt_includes})
      string(TOUPPER ${winkit_ucrt_include} winkit_ucrt_include_macro)
      string(REGEX  REPLACE "[/\.]" "_" winkit_ucrt_include_macro
          ${winkit_ucrt_include_macro})
      file(APPEND ${crtheaders_h} "#define ${winkit_ucrt_include_macro} \
\"${winkit_ucrt_includes_dir}/${winkit_ucrt_include}\"\n")
    endforeach()
    include_directories(${PROJECT_BINARY_DIR}/include)
    list(APPEND libedit_extra_includes ${PROJECT_BINARY_DIR}/include)
endif()

#
# compat regex
#
set(regex_source
    compat/regex/regcomp.c
    compat/regex/regerror.c
    compat/regex/regexec.c
    compat/regex/regfree.c)
if(WIN32)
    include_directories(compat/regex/include)
    add_library(regex STATIC ${regex_source})
    list(APPEND libedit_extra_libs regex)
    install(TARGETS regex DESTINATION lib EXPORT edit-targets)
endif()

#
# compat win32
#
if(WIN32)
    file(GLOB win32_source compat/win32/*.c)
    include_directories(compat/win32/include)
    add_library(compat STATIC ${win32_source})
    install(TARGETS compat DESTINATION lib EXPORT edit-targets)
    list(APPEND libedit_extra_libs compat crypt32 secur32 shlwapi userenv wsock32 ws2_32)
    list(APPEND libedit_extra_includes ${PROJECT_SOURCE_DIR}/compat/win32/include)
endif()

#
# compat pdcurses and terminfo
#
if(WIN32)
    include_directories(compat/pdcurses/include)
    file(GLOB pdcurses_source compat/pdcurses/*.c compat/pdcurses/vt/*.c)
    add_library(pdcurses STATIC ${pdcurses_source})
    install(TARGETS pdcurses DESTINATION lib EXPORT edit-targets)
    list(APPEND libedit_extra_libs pdcurses)

    file(GLOB terminfo_source compat/terminfo/*.c)
    add_library(terminfo STATIC ${terminfo_source})
    install(TARGETS terminfo DESTINATION lib EXPORT edit-targets)
    list(APPEND libedit_extra_libs terminfo)
else()
    list(APPEND libedit_extra_libs ncurses)
endif()

#
# libedit generated source
#
if(WIN32)
    include_directories(libedit/include)
else()
    set(libedit_gen_src
        ${PROJECT_SOURCE_DIR}/libedit/vi.c
        ${PROJECT_SOURCE_DIR}/libedit/emacs.c
        ${PROJECT_SOURCE_DIR}/libedit/common.c)
    set(libedit_gen_hdr
        ${PROJECT_BINARY_DIR}/include/vi.h
        ${PROJECT_BINARY_DIR}/include/emacs.h
        ${PROJECT_BINARY_DIR}/include/common.h)
    set(libedit_gen_hdr2
        ${PROJECT_BINARY_DIR}/include/fcns.h
        ${PROJECT_BINARY_DIR}/include/func.h
        ${PROJECT_BINARY_DIR}/include/help.h)

    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/include/vi.h
                     COMMAND ${PROJECT_SOURCE_DIR}/libedit/scripts/makelist
                     ARGS -h ${PROJECT_SOURCE_DIR}/libedit/vi.c > ${PROJECT_BINARY_DIR}/include/vi.h
                     DEPENDS ${PROJECT_SOURCE_DIR}/libedit/vi.c VERBATIM)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/include/emacs.h
                     COMMAND ${PROJECT_SOURCE_DIR}/libedit/scripts/makelist
                     ARGS -h ${PROJECT_SOURCE_DIR}/libedit/emacs.c > ${PROJECT_BINARY_DIR}/include/emacs.h
                     DEPENDS ${PROJECT_SOURCE_DIR}/libedit/emacs.c VERBATIM)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/include/common.h
                     COMMAND ${PROJECT_SOURCE_DIR}/libedit/scripts/makelist
                     ARGS -h ${PROJECT_SOURCE_DIR}/libedit/common.c > ${PROJECT_BINARY_DIR}/include/common.h
                     DEPENDS ${PROJECT_SOURCE_DIR}/libedit/common.c VERBATIM)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/include/fcns.h
                     COMMAND ${PROJECT_SOURCE_DIR}/libedit/scripts/makelist
                     ARGS -fh ${libedit_gen_hdr} > ${PROJECT_BINARY_DIR}/include/fcns.h
                     DEPENDS ${libedit_gen_hdr} VERBATIM)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/include/func.h
                     COMMAND ${PROJECT_SOURCE_DIR}/libedit/scripts/makelist
                     ARGS -fc ${libedit_gen_hdr} > ${PROJECT_BINARY_DIR}/include/func.h
                     DEPENDS ${libedit_gen_hdr} VERBATIM)
    add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/include/help.h
                     COMMAND ${PROJECT_SOURCE_DIR}/libedit/scripts/makelist
                     ARGS -bh ${libedit_gen_src} > ${PROJECT_BINARY_DIR}/include/help.h
                     DEPENDS ${libedit_gen_src} VERBATIM)

    include_directories(${PROJECT_BINARY_DIR}/include)
    list(APPEND libedit_extra_source ${libedit_gen_hdr} ${libedit_gen_hdr2})
endif()

#
# libedit
#
file(GLOB libedit_source libedit/*.c)
include_directories(libedit)
add_library(edit STATIC ${libedit_source} ${libedit_extra_source})
install(TARGETS edit DESTINATION lib EXPORT edit-targets)

#
# demo
#
add_executable(demo demo/demo.c)
target_link_libraries(demo edit ${libedit_extra_libs})

#
# export
#
get_directory_property(has_parent PARENT_DIRECTORY)
if(has_parent)
set(edit_libs edit ${libedit_extra_libs} PARENT_SCOPE)
set(edit_includes ${libedit_extra_includes} ${PROJECT_SOURCE_DIR}/libedit PARENT_SCOPE)
endif(has_parent)
