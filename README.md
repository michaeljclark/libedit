# libedit

cross-platform port of NetBSD libedit to Linux and Windows.

This is CMake port of the NetBSD Editline library (libedit).
This Berkeley-style licensed command line editor library provides
generic line editing, history, and tokenization functions, similar
to those found in GNU Readline.

This port of libedit bundles the following dependencies:

- Public Domain Curses (Modified), aka PDCursesMod, an
  X/Open curses implementation.
- terminfo implementation using the NetBSD tparm implementation.
- BSD compatibility layer for Win32 from openssh-portable.
- BSD regex implementation.

## Building

### Supported Platforms

- Microsoft Windows
- Linux

### Building on Linux

```
cmake -B build_linux -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build_linux
```

### Building on Windows

```
cmake -B build_win32 -G "Visual Studio 17 2022" -A x64
cmake --build build_win32 --config RelWithDebInfo
```
