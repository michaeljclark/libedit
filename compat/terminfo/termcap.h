#pragma once

#include <stddef.h>

int tgetent(char *bp, const char *name);
int tgetflag(char *id);
int tgetnum(char *id);
char *tgetstr(const char *id, char **area);
char *tgoto(const char *cap, int col, int row);
char *tparm(const char *fmt, ...);
int tputs(const char *str, int affcnt, int (*putc)(int));
