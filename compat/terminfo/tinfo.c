/*
 * Copyright (c) 2009, 2011, 2013 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

/* Print a parameter if needed */
static size_t printparam(char **dst, char p, bool *nop)
{
	if (*nop) {
		*nop = false;
		return 0;
	}

	*(*dst)++ = '%';
	*(*dst)++ = 'p';
	*(*dst)++ = '0' + p;
	return 3;
}

/* Convert a termcap character into terminfo equivalents */
static size_t printchar(char **dst, const char **src)
{
	char v;
	size_t l;

	l = 4;
	v = *++(*src);
	if (v == '\\') {
		v = *++(*src);
		switch (v) {
		case '0':
		case '1':
		case '2':
		case '3':
			v = 0;
			while (isdigit((unsigned char) **src))
				v = 8 * v + (*(*src)++ - '0');
			(*src)--;
			break;
		case '\0':
			v = '\\';
			break;
		}
	} else if (v == '^')
		v = *++(*src) & 0x1f;
	*(*dst)++ = '%';
	if (isgraph((unsigned char )v) &&
	    v != ',' && v != '\'' && v != '\\' && v != ':')
	{
		*(*dst)++ = '\'';
		*(*dst)++ = v;
		*(*dst)++ = '\'';
	} else {
		*(*dst)++ = '{';
		if (v > 99) {
			*(*dst)++ = '0'+ v / 100;
			l++;
		}
		if (v > 9) {
			*(*dst)++ = '0' + ((int) (v / 10)) % 10;
			l++;
		}
		*(*dst)++ = '0' + v % 10;
		*(*dst)++ = '}';
	}
	return l;
}

/* Convert termcap commands into terminfo commands */
static const char fmtB[] = "%p0%{10}%/%{16}%*%p0%{10}%m%+";
static const char fmtD[] = "%p0%p0%{2}%*%-";
static const char fmtIf[] = "%p0%p0%?";
static const char fmtThen[] = "%>%t";
static const char fmtElse[] = "%+%;";

/* sourced from NetBSD libterminfo/termcap.c:strval */
char* tc_tinfo_strval(const char *val)
{
	char *info, *ip, c, p;
	const char *ps, *pe;
	bool nop;
	size_t len, l;

	len = 1024; /* no single string should be bigger */
	info = ip = malloc(len);
	if (info == NULL)
		return 0;

	/* Move the = */
	*ip++ = *val++;

	/* Set ps and pe to point to the start and end of the padding */
	if (isdigit((unsigned char)*val)) {
		for (ps = pe = val;
		     isdigit((unsigned char)*val) || *val == '.';
		     val++)
			pe++;
		if (*val == '*') {
			val++;
			pe++;
		}
	} else
		ps = pe  = NULL;

	nop = false;
	l = 0;
	p = 1;
	for (; *val != '\0'; val++) {
		if (l + 2 > len)
			goto elen;
		if (*val != '%') {
			if (*val == ',') {
				if (l + 3 > len)
					goto elen;
				*ip++ = '\\';
				l++;
			}
			*ip++ = *val;
			l++;
			continue;
		}
		switch (c = *++(val)) {
		case 'B':
			if (l + sizeof(fmtB) > len)
				goto elen;
			memcpy(ip, fmtB, sizeof(fmtB) - 1);
			/* Replace the embedded parameters with real ones */
			ip[2] += p;
			ip[19] += p;
			ip += sizeof(fmtB) - 1;
			l += sizeof(fmtB) - 1;
			nop = true;
			continue;
		case 'D':
			if (l + sizeof(fmtD) > len)
				goto elen;
			memcpy(ip, fmtD, sizeof(fmtD) - 1);
			/* Replace the embedded parameters with real ones */
			ip[2] += p;
			ip[5] += p;
			ip += sizeof(fmtD) - 1;
			l += sizeof(fmtD) - 1;
			nop = true;
			continue;
		case 'r':
			/* non op as switched below */
			break;
		case '2': /* FALLTHROUGH */
		case '3': /* FALLTHROUGH */
		case 'd':
			if (l + 7 > len)
				goto elen;
			l += printparam(&ip, p, &nop);
			*ip++ = '%';
			if (c != 'd') {
				*ip++ = c;
				l++;
			}
			*ip++ = 'd';
			l += 2;
			break;
		case '+':
			if (l + 13 > len)
				goto elen;
			l += printparam(&ip, p, &nop);
			l += printchar(&ip, &val);
			*ip++ = '%';
			*ip++ = c; 
			*ip++ = '%';
			*ip++ = 'c';
			l += 7;
			break;
		case '>':
			if (l + sizeof(fmtIf) + sizeof(fmtThen) +
			    sizeof(fmtElse) + (6 * 2) > len)
				goto elen;

			memcpy(ip, fmtIf, sizeof(fmtIf) - 1);
			/* Replace the embedded parameters with real ones */
			ip[2] += p;
			ip[5] += p;
			ip += sizeof(fmtIf) - 1;
			l += sizeof(fmtIf) - 1;
			l += printchar(&ip, &val);
			memcpy(ip, fmtThen, sizeof(fmtThen) - 1);
			ip += sizeof(fmtThen) - 1;
			l += sizeof(fmtThen) - 1;
			l += printchar(&ip, &val);
			memcpy(ip, fmtElse, sizeof(fmtElse) - 1);
			ip += sizeof(fmtElse) - 1;
			l += sizeof(fmtElse) - 1;
			l += 16;
			nop = true;
			continue;
		case '.':
			if (l + 6 > len)
				goto elen;
			l += printparam(&ip, p, &nop);
			*ip++ = '%';
			*ip++ = 'c';
			l += 2;
			break;
		default:
			/* Hope it matches a terminfo command. */
			*ip++ = '%';
			*ip++ = c;
			l += 2;
			if (c == 'i')
				continue;
			break;
		}
		/* Swap p1 and p2 */
		p = 3 - p;
	}

	/* \E\ is valid termcap.
	 * We need to escape the final \ for terminfo. */
	if (l > 2 && info[l - 1] == '\\' &&
	    (info[l - 2] != '\\' && info[l - 2] != '^'))
	{
		if (l + 1 > len)
			goto elen;
		*ip++ = '\\';
	}

	/* Add our padding at the end. */
	if (ps != NULL) {
		size_t n = (size_t)(pe - ps);
		if (l + n + 4 > len)
			goto elen;
		*ip++ = '$';
		*ip++ = '<';
		strncpy(ip, ps, n);
		ip += n;
		*ip++ = '/';
		*ip++ = '>';
	}

	*ip = '\0';
	return info;

elen:
	free(info);
	errno = ENOMEM;
	return NULL;
}
