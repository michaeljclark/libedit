#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <stdio.h>")
#endif

#include "crtheaders.h"
#include STDIO_H

#ifndef W32_COMPAT_IMPL
#define fopen w32_fopen_utf8
#define fgets w32_fgets
#define setvbuf w32_setvbuf
#define fdopen(a,b)	w32_fdopen((a), (b))
#define rename w32_rename
#endif

/* stdio.h additional definitions */
#define popen _popen
#define pclose _pclose

/* stdio.h overrides */
FILE* w32_fopen_utf8(const char *, const char *);
char* w32_fgets(char *str, int n, FILE *stream);
int w32_setvbuf(FILE *stream,char *buffer, int mode, size_t size);
FILE* w32_fdopen(int fd, const char *mode);
int w32_rename(const char *old_name, const char *new_name);

/* required by readline.c */
typedef long long off_t;
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);

/* required by el.c */
typedef int ssize_t;
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
