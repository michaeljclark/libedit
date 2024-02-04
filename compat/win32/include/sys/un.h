#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <sys/un.h>")
#endif

struct	sockaddr_un {
	short	sun_family;		/* AF_UNIX */
	char	sun_path[108];		/* path name (gag) */
};

