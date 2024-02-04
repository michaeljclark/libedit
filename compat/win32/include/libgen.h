#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <libgen.h>")
#endif

char *basename(char *path);
