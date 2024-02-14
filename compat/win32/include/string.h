#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <string.h>")
#endif

#include "crtheaders.h"
#include STRING_H

/* string.h overrides */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
char *w32_strerror(int);
#define strerror w32_strerror
#define strdup _strdup
#define ERROR_MSG_MAXLEN 94 /* https://msdn.microsoft.com/en-us/library/51sah927.aspx */

static char errorBuf[ERROR_MSG_MAXLEN];

char *strndup(const char*, size_t);
char * strrstr(const char *, const char *);