/*
 * PLEASE LICENSE 2023, Michael Clark <michaeljclark@mac.com>
 *
 * All rights to this work are granted for all purposes, with exception of
 * author's implied right of copyright to defend the free use of this work.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#include "tparser.h"
#include "termcap.h"

#define VA_ARGS(...) , ##__VA_ARGS__
#define tc_error(fmt,...) fprintf(stderr, fmt "\n" VA_ARGS(__VA_ARGS__))
#define tc_debug(fmt,...) if(tp.debug) printf(fmt "\n" VA_ARGS(__VA_ARGS__))
#define tc_verbose(fmt,...) printf(fmt "\n" VA_ARGS(__VA_ARGS__))

static tc_parser tp;
static int initialized;
static int opt_debug;
static const char *termcap_file = "data/termcap";
static tc_symbol sym_tc;
static lhmap cap_map;

static int tinfo_init()
{
	if (initialized) return 0;
	if (tc_init(&tp, opt_debug) < 0) return -1;
	if (tc_read(&tp, termcap_file) < 0) return -1;
	sym_tc = tc_make_symbol(&tp, "tc", 2);
	lhmap_init_ex(&cap_map, &tp, sizeof(tc_symbol), sizeof(tc_cap_obj),
		tc_table_init_size, tc_symbol_hash_fn, tc_symbol_compare_fn);
	initialized = 1;
	return 0;
}

int tgetent(char *bp, const char *name)
{
	tc_symbol sym_name;
	tc_cap_ref *cap_ref;
	tc_cap_list *cap_list;
	char tmp[1024];

	if (tinfo_init() < 0) return -1;

	sym_name = tc_make_symbol(&tp, name, strlen(name));
	cap_ref = lhmap_get(&tp.term_map, &sym_name);
	if (!cap_ref) return 0;

	do
	{
		cap_list = array_buffer_get(&tp.cap_list,
			sizeof(tc_cap_list), cap_ref->cap_idx);
		for(lhmap_iter i = lhmap_iter_begin(&cap_list->caps);
			lhmap_iter_neq(i, lhmap_iter_end(&cap_list->caps));
			i = lhmap_iter_next(i))
		{
			tc_symbol *k = (tc_symbol*)lhmap_iter_key(i);
			tc_cap_obj *v = (tc_cap_obj*)lhmap_iter_val(i);
			if (tc_symbol_compare_fn(&cap_list->caps, k, &sym_tc)) continue;
			tc_obj_str(&tp, tmp, sizeof(tmp), v);
			tc_debug("tgetent: %s", tmp);
			if (v->type == tc_cap_type_string) {
				/* convert strings from termcap to terminfo format */
				tc_cap_obj cv = { 0 };
				char *str = tc_get_string(&tp, v->str_val);
				char *cvt = tc_tinfo_strval(str);
				cv.type = tc_cap_type_string;
				cv.str_key = v->str_key;
				cv.str_val = tc_make_symbol(&tp, cvt, strlen(cvt));
				lhmap_insert(&cap_map, lhmap_iter_end(&cap_map), k, &cv);
				free(str);
				free(cvt);
			} else {
				lhmap_insert(&cap_map, lhmap_iter_end(&cap_map), k, v);
			}
		}
		tc_cap_obj *v = lhmap_get(&cap_list->caps, &sym_tc);
		cap_ref = (v && v->type == tc_cap_type_string) ?
			lhmap_get(&tp.term_map, &v->str_val) : NULL;
	} while (cap_ref);

	return 1;
}

int tgetflag(char *id)
{
	tc_symbol sym_id = tc_make_symbol(&tp, id, strlen(id));
	tc_cap_obj *v = (tc_cap_obj *)lhmap_get(&cap_map, &sym_id);
	return (v && v->type == tc_cap_type_boolean);
}

int tgetnum(char *id)
{
	tc_symbol sym_id = tc_make_symbol(&tp, id, strlen(id));
	tc_cap_obj *v = (tc_cap_obj *)lhmap_get(&cap_map, &sym_id);
	return (v && v->type == tc_cap_type_number) ? v->int_val : -1;
}

char *tgetstr(const char *id, char **area)
{
	tc_symbol sym_id;
	tc_cap_obj *v;
	size_t length;
	char *data, *str;

	if (area == NULL || *area == NULL) return NULL;

	sym_id = tc_make_symbol(&tp, id, strlen(id));
	v = (tc_cap_obj *)lhmap_get(&cap_map, &sym_id);
	if (!v || v->type != tc_cap_type_string) return NULL;

	length = v->str_val.length;
	data = storage_buffer_get(&tp.str_tab, v->str_val.offset);
	str = *area;
	memcpy(str, data, length);
	str[length] = '\0';
	*area = *area + length + 1;

	return str;
}

char *tgoto(const char *cap, int col, int row)
{
	return tparm(cap, row, col);
}

int tputs(const char *str, int affcnt, int (*putc)(int))
{
	while (*str) putc(*str++);
	return 0;
}
