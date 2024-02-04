#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <sys/resource.h>")
#endif

/* Compatibility header to avoid lots of #ifdef _WIN32's in includes.h */
