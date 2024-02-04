#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <sys/uio.h>")
#endif

struct iovec
{
	void *iov_base;
	size_t iov_len;
};
