/*
* Author: Manoj Ampalam <manoj.ampalam@microsoft.com>
*
* Compatibility header to give us pwd-like functionality on Win32
* A lot of passwd fields are not applicable in Windows, neither are some API calls based on this structure
* Ideally, usage of this structure needs to be replaced in core SSH code to an ssh_user interface,
* that each platform can extend and implement.
*/

#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <pwd.h>")
#endif

#ifndef W32_COMPAT_IMPL
#define getpwuid w32_getpwuid
#define getpwnam w32_getpwnam
#define getuid w32_getuid
#endif

#include "sys/types.h"
#include "Windows.h"

struct passwd {
	char	*pw_name;	/* user's login name */
	char	*pw_passwd;	/* password? */
	char	*pw_gecos;	/* ??? */
	uid_t	pw_uid;		/* numerical user ID */
	gid_t	pw_gid;		/* numerical group ID */
	char	*pw_dir;	/* initial working directory */
	char	*pw_shell;	/* path to shell */
};

/*start - declarations not applicable in Windows */
uid_t w32_getuid(void);
gid_t getgid(void);
uid_t geteuid(void);
gid_t getegid(void);
int setuid(uid_t uid);
int setgid(gid_t gid);
int seteuid(uid_t uid);
int setegid(gid_t gid);
char *user_from_uid(uid_t uid, int nouser);
struct passwd *getpwent(void);
void setpwent(void);

/*end - declarations not applicable in Windows */

struct passwd *w32_getpwuid(uid_t uid);
struct passwd *w32_getpwnam(const char *username);
struct passwd *getpwent(void);
void endpwent(void);
char *get_username(const PSID sid);
