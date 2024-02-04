#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <time.h>")
#endif

#include "crtheaders.h"
#include TIME_H

#ifndef W32_COMPAT_IMPL
#define localtime w32_localtime
#define ctime w32_ctime
#endif

struct tm *localtime_r(const time_t *, struct tm *);
struct tm *w32_localtime(const time_t* sourceTime);
char *w32_ctime(const time_t* sourceTime);
