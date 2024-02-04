#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <stdlib.h>")
#endif

#include "crtheaders.h"
#include STDLIB_H

#ifndef W32_COMPAT_IMPL
#define system w32_system
#endif

#define environ _environ
void freezero(void *, size_t);
int setenv(const char *name, const char *value, int rewrite);
int w32_system(const char *command);
char* realpath(const char *pathname, char *resolved);

/* required by readline.c */
int mkstemp(char *template);
