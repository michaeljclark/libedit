// direntry functions in Windows platform like Ubix/Linux
// opendir(), readdir(), closedir().
// 	NT_DIR * nt_opendir(char *name) ;
// 	struct nt_dirent *nt_readdir(NT_DIR *dirp);
// 	int nt_closedir(NT_DIR *dirp) ;

#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <dirent.h>")
#endif

#include <fcntl.h>
#include "../misc_internal.h"

struct dirent {
	int            d_ino;       /* Inode number */
	char           d_name[PATH_MAX]; /* Null-terminated filename */
};

typedef struct DIR_ DIR;

DIR * opendir(const char*);
int closedir(DIR*);
struct dirent *readdir(void*);
