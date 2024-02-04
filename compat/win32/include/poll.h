#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <poll.h>")
#endif

#include "sys/types.h"
#include "sys/socket.h"

/* created to #def out decarations in open-bsd.h (that are defined in winsock2.h) */

int poll(struct pollfd *, nfds_t, int);