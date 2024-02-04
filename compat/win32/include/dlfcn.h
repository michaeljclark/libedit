#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <dlfcn.h>")
#endif

#include <Windows.h>
#define RTLD_NOW 0

HMODULE dlopen(const char *filename, int flags);
int dlclose(HMODULE handle);
void * dlsym(HMODULE handle, const char *symbol);
char * dlerror();
