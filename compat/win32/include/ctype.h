#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <ctype.h>")
#endif

#include "crtheaders.h"
#include CTYPE_H

#define isascii __isascii

