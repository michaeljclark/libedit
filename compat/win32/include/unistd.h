/*
* Author: Manoj Ampalam <manoj.ampalam@microsoft.com>
*
* POSIX header and needed function definitions
*/
#pragma once

#if WIN32_COMPAT_DEBUG
#pragma message("#include <unistd.h>")
#endif

#include <stddef.h>
#include "sys/types.h"
#include "fcntl.h"
#include "spawn.h"

#ifndef W32_COMPAT_IMPL
#define pipe w32_pipe
#define read w32_read
#define write w32_write
#define writev w32_writev
#define isatty(a)   w32_isatty((a))
#define close w32_close
#define dup w32_dup
#define dup2 w32_dup2
#define sleep(sec) Sleep(1000 * sec)
#define alarm w32_alarm
#define lseek w32_lseek
#define getdtablesize() MAX_FDS
#define gethostname w32_gethostname
#define fsync(a) w32_fsync((a))
#define symlink w32_symlink
#define chown w32_chown
#define fchown w32_fchown
#define unlink w32_unlink
#define rmdir w32_rmdir
#define chdir w32_chdir
#define getcwd w32_getcwd
#define readlink w32_readlink
#define link w32_link
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SFTP_SERVER_LOG_FD STDERR_FILENO+1

int w32_ftruncate(int, off_t);
#define ftruncate(a, b) w32_ftruncate((a), (b))
int w32_pipe(int *pfds);
int w32_read(int fd, void *dst, size_t max);
int w32_write(int fd, const void *buf, size_t max);
int w32_writev(int fd, const struct iovec *iov, int iovcnt);
int w32_isatty(int fd);
int w32_close(int fd);
int w32_dup(int oldfd);
int w32_dup2(int oldfd, int newfd);
unsigned int w32_alarm(unsigned int seconds);
long w32_lseek(int fd, unsigned __int64 offset, int origin);
int w32_gethostname(char *, size_t);
int w32_fsync(int fd);
int w32_symlink(const char *target, const char *linkpath);
int w32_chown(const char *pathname, unsigned int owner, unsigned int group);
int w32_fchown(int fd, unsigned int owner, unsigned int group);
int w32_unlink(const char *path);
int w32_rmdir(const char *pathname);
int w32_chdir(const char *dirname);
char *w32_getcwd(char *buffer, int maxlen);
int w32_readlink(const char *path, char *link, int linklen);
int w32_link(const char *oldpath, const char *newpath);

int getpeereid(int, uid_t*, gid_t*);
int daemon(int nochdir, int noclose);
char *crypt(const char *key, const char *salt);
int chroot(const char *path);

/* 
 * readpassphrase.h definitions 
 * cannot create a separate header due to the way
 * its included in openbsdcompat.h
 */

#define RPP_ECHO_OFF    0x00		/* Turn off echo (default). */
#define RPP_ECHO_ON     0x01		/* Leave echo on. */
#define RPP_REQUIRE_TTY 0x02		/* Fail if there is no tty. */
#define RPP_FORCELOWER  0x04		/* Force input to lower case. */
#define RPP_FORCEUPPER  0x08		/* Force input to upper case. */
#define RPP_SEVENBIT    0x10		/* Strip the high bit from input. */
#define RPP_STDIN       0x20		/* Read from stdin, not /dev/tty */

char * readpassphrase(const char *, char *, size_t, int);