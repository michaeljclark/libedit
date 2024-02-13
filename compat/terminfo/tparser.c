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
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "tparser.h"

#define VA_ARGS(...) , ##__VA_ARGS__
#define tc_error(fmt,...) fprintf(stderr, fmt "\n" VA_ARGS(__VA_ARGS__))
#define tc_debug(fmt,...) if(tp->debug) printf(fmt "\n" VA_ARGS(__VA_ARGS__))
#define tc_verbose(fmt,...) printf(fmt "\n" VA_ARGS(__VA_ARGS__))

static char* char_str(char *tmp, size_t n, char c)
{
	switch (c) {
	case '\033': snprintf(tmp, n, "\\E (#x%02x)", c); break;
	case '\n': snprintf(tmp, n, "\\n (#x%02x)", c); break;
	case '\r': snprintf(tmp, n, "\\r (#x%02x)", c); break;
	case '\t': snprintf(tmp, n, "\\t (#x%02x)", c); break;
	case '\b': snprintf(tmp, n, "\\b (#x%02x)", c); break;
	case '\f': snprintf(tmp, n, "\\f (#x%02x)", c); break;
	case 127: snprintf(tmp, n, "^? (#x%02x)", c); break;
	default:
		if (c < 32) {
			snprintf(tmp, n, "^%c (#x%02x)", '@' + c, c);
		} else {
			snprintf(tmp, n, "'%c' (#x%02x)", c, c);
		}
		break;
	}
	return tmp;
}

size_t tc_sym_str(tc_parser *tp, char *tmp, size_t n, tc_symbol sym)
{
	const char *str = (char*)storage_buffer_get(&tp->str_tab, sym.offset);
	const char *end = str + sym.length;
	size_t len = 0;
	while (str != end) {
		char c = *str++;
		switch(c) {
		case '\033': len += snprintf(tmp + len, n - len, "\\E"); break;
		case '\\': len += snprintf(tmp + len, n - len, "\\\\"); break;
		case '^': len += snprintf(tmp + len, n - len, "\\^"); break;
		case ':': len += snprintf(tmp + len, n - len, "\\:"); break;
		case '.': len += snprintf(tmp + len, n - len, "\\."); break;
		case '\n': len += snprintf(tmp + len, n - len, "\\n"); break;
		case '\r': len += snprintf(tmp + len, n - len, "\\r"); break;
		case '\t': len += snprintf(tmp + len, n - len, "\\t"); break;
		case '\b': len += snprintf(tmp + len, n - len, "\\b"); break;
		case '\f': len += snprintf(tmp + len, n - len, "\\f"); break;
		case 127: len += snprintf(tmp + len, n - len, "^?"); break;
		default:
			if (c < 32) {
				len += snprintf(tmp + len, n - len, "^%c", '@' + c);
			} else {
				len += snprintf(tmp + len, n - len, "%c", c);
			}
			break;
		}
		if (len >= n-1) goto out;
	}
out:
	return len;
}

int tc_obj_str(tc_parser *tp, char *tmp, size_t n, tc_cap_obj *obj)
{
	size_t len;
	switch (obj->type) {
	case tc_cap_type_boolean:
		len = tc_sym_str(tp, tmp, n, obj->str_key);
		break;
	case tc_cap_type_number:
		len = tc_sym_str(tp, tmp, n, obj->str_key);
		if (len >= n-1) goto out;
		len += snprintf(tmp + len, n - len, "#%d", obj->int_val);
		break;
	case tc_cap_type_string:
		len = tc_sym_str(tp, tmp, n, obj->str_key);
		if (len >= n-1) goto out;
		len += snprintf(tmp + len, n - len, "=");
		if (len >= n-1) goto out;
		if (obj->pad_delay) {
			len += snprintf(tmp + len, n - len, "%d%s",
				obj->pad_delay, obj->pad_prop ? "*" : "");
			if (len >= n-1) goto out;
		}
		len += tc_sym_str(tp, tmp + len, n - len, obj->str_val);
		break;
	default:
		len = snprintf(tmp, n, "(invalid object)");
		break;
	}
out:
	return len;
}

size_t tc_symbol_hash_fn(lhmap *h, void *key)
{
	tc_parser *tp = (tc_parser*)lhmap_userdata(h);
	tc_symbol *sym = (tc_symbol*)key;
	const char *str = (char*)storage_buffer_get(&tp->str_tab, sym->offset);
	const char *end = str + sym->length;
	const size_t fnv_prime = 0x100000001b3;
	size_t hash = 0xcbf29ce484222325;
	while (str != end) {
		hash ^= (unsigned char)(*str++);
		hash *= fnv_prime;
	}
	return hash;
}

int tc_symbol_compare_fn(lhmap *h, void *key1, void *key2)
{
	tc_parser *tp = (tc_parser*)lhmap_userdata(h);
	tc_symbol *sym1 = (tc_symbol*)key1, *sym2 = (tc_symbol*)key2;
	const void *str1 = storage_buffer_get(&tp->str_tab, sym1->offset);
	const void *str2 = storage_buffer_get(&tp->str_tab, sym2->offset);
	return (sym1->length == sym2->length) &&
		memcmp(str1, str2, sym1->length) == 0;
}

char* tc_get_string(tc_parser *tp, tc_symbol sym)
{
	char *str = malloc(sym.length + 1);
	memcpy(str, storage_buffer_get(&tp->str_tab, sym.offset), sym.length);
	str[sym.length] = '\0';
	return str;
}

tc_symbol tc_make_symbol(tc_parser *tp, const char *str, size_t len)
{
	tc_symbol sym;
	sym.length = (int)len;
	sym.offset = (int)storage_buffer_alloc(&tp->str_tab, sym.length, 1);
	memcpy(storage_buffer_get(&tp->str_tab, sym.offset), str, sym.length);
	return sym;
}

static void tc_copy_symbol(tc_parser *tp, tc_symbol *sym)
{
	sym->length = (int)storage_buffer_size(&tp->cur_str);
	sym->offset = (int)storage_buffer_alloc(&tp->str_tab, sym->length, 1);
	memcpy(storage_buffer_get(&tp->str_tab, sym->offset),
		storage_buffer_get(&tp->cur_str, 0), sym->length);
	storage_buffer_reset(&tp->cur_str);
}

static void tc_reset_object(tc_parser *tp)
{
	memset(&tp->cur_obj, 0, sizeof(tp->cur_obj));
}

static void tc_store_object(tc_parser *tp, char *tmp, size_t n)
{
	if (tp->cur_obj.str_key.length == 0) return;
	tc_cap_list *cap_list = array_buffer_get(&tp->cap_list,
		sizeof(tc_cap_list), tp->cur_cap.cap_idx);
	lhmap_insert(&cap_list->caps, lhmap_iter_end(&cap_list->caps),
		&tp->cur_obj.str_key, &tp->cur_obj);
	if (tp->debug) tc_obj_str(tp, tmp, n, &tp->cur_obj);
	tc_reset_object(tp);
	tc_debug("termcap: cap: %s, line:%u, col:%u", tmp, tp->ln, tp->col);
}

static void tc_alloc_map(tc_parser *tp)
{
	tc_cap_list cap_list;
	lhmap_init_ex(&cap_list.caps, tp, sizeof(tc_symbol), sizeof(tc_cap_obj),
		tc_table_init_size, tc_symbol_hash_fn, tc_symbol_compare_fn);
	tp->cur_cap.cap_idx = array_buffer_add(&tp->cap_list,
		sizeof(tc_cap_list), &cap_list.caps);
}

static int is_crlf_begin(tc_parser *tp, char c)
{
	int is_cr = (c == '\r');
	if (is_cr) tp->crlf = 1;
	return is_cr;
}

static int is_crlf_end(tc_parser *tp, char c)
{
	int is_lf = (c == '\n');
	if (is_lf) {
		tp->crlf = 0;
		tp->col = 0;
		tp->ln++;
	}
	return is_lf;
}

static int is_crlf_error(tc_parser *tp, char c)
{
	int crlf = tp->crlf;
	tp->crlf = 0;
	return crlf;
}

#define tc_parse_debug(fmt,...) \
	if (tp->debug) fprintf(stderr, "termcap: " fmt " %s line:%u col:%u\n" \
		VA_ARGS(__VA_ARGS__), char_str(tmp, sizeof(tmp), c), ln, col)
#define tc_parse_error(fmt,...) \
	fprintf(stderr, "termcap: " fmt " %s line:%u col:%u\n" \
		VA_ARGS(__VA_ARGS__), char_str(tmp, sizeof(tmp), c), ln, col)

static int tc_parse(tc_parser *tp, char c)
{
	char tmp[1024];
	int ln = tp->ln, col = tp->col;
	if (is_crlf_begin(tp, c)) return 0;
redo:
	switch (tp->state) {
	case tc_whitespace:
		if (is_crlf_end(tp, c)) {
			return 0;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("whitespace: invalid crlf");
			return -1;
		} else if (isspace(c)) {
			/* no-op */
		} else if (c == '#') {
			tp->state = tc_comment;
		} else if (isprint(c)) {
			tp->state = tp->restore_state;
			goto redo;
		} else {
			tc_parse_error("whitespace: unexpected character");
			return -1;
		}
		break;
	case tc_comment:
		if (is_crlf_end(tp, c)) {
			tp->state = tc_whitespace;
			return 0;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("comment: invalid crlf");
			return -1;
		} else if (isprint(c) || isspace(c)) {
			/* no-op */
		} else {
			tc_parse_error("comment: unexpected character");
			return -1;
		}
		break;
	case tc_term:
		if (is_crlf_end(tp, c)) {
			tc_parse_error("term: unexpected linefeed");
			return -1;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("term: invalid crlf");
			return -1;
		} else if (c == '\\') {
			tp->restore_state = tc_term;
			tp->state = tc_escape;
		} else if (c == '|' || c == ':') {
			tc_copy_symbol(tp, &tp->cur_term);
			if (tp->cur_term.length > 0) {
				 lhmap_insert(&tp->term_map, lhmap_iter_end(&tp->term_map),
					&tp->cur_term, &tp->cur_cap);
				 if (tp->debug) tc_sym_str(tp, tmp, sizeof(tmp), tp->cur_term);
				 tc_debug("termcap: term: %s -> idx:%u, line:%u, col:%u",
					tmp, tp->cur_cap.cap_idx, ln, col);
			}
			if (c == ':') {
				tc_reset_object(tp);
				tp->state = tc_val_key;
			}
		} else if (isprint(c) || isspace(c)) {
			/* skip leading whitespace */
			if (storage_buffer_size(&tp->cur_str) > 0 || !isspace(c)) {
				storage_buffer_append(&tp->cur_str, &c, 1);
			}
		} else {
			tc_parse_error("term: unexpected character");
			return -1;
		}
		break;
	case tc_val_key:
		if (is_crlf_end(tp, c)) {
			tc_alloc_map(tp);
			tp->restore_state = tc_term;
			tp->state = tc_whitespace;
			return 0;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("val_key: invalid crlf");
			return -1;
		} else if (c == '\\') {
			tp->restore_state = tc_val_key;
			tp->state = tc_escape;
		} else if (c == '.') {
			tp->state = tc_val_skip;
		} else if (c == ':') {
			tc_copy_symbol(tp, &tp->cur_obj.str_key);
			tc_store_object(tp, tmp, sizeof(tmp));
		} else if (c == '#' && storage_buffer_size(&tp->cur_str) > 0) {
			tc_copy_symbol(tp, &tp->cur_obj.str_key);
			tp->cur_obj.type = tc_cap_type_number;
			tp->state = tc_val_num;
		} else if (c == '=') {
			tc_copy_symbol(tp, &tp->cur_obj.str_key);
			tp->cur_obj.type = tc_cap_type_string;
			tp->state = tc_val_string;
		} else if (isprint(c) || isspace(c)) {
			/* skip leading whitespace */
			if (storage_buffer_size(&tp->cur_str) > 0 || !isspace(c)) {
				storage_buffer_append(&tp->cur_str, &c, 1);
			}
		} else {
			tc_parse_error("val_key: unexpected character");
			return -1;
		}
		break;
	case tc_val_skip:
		if (is_crlf_end(tp, c)) {
			tc_parse_debug("val_skip: unexpected linefeed");
			tp->restore_state = tc_term;
			tp->state = tc_whitespace;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("val_skip: invalid crlf");
			return -1;
		} else if (c == ':') {
			tp->state = tc_val_key;
		} else if (isprint(c) || isspace(c)) {
			/* no-op */
		} else if (c == ':') {
			tp->state = tc_val_key;
		} else {
			tc_parse_error("val_skip: unexpected character");
			return -1;
		}
		break;
	case tc_val_num:
		if (is_crlf_end(tp, c)) {
			tc_parse_debug("val_num: unexpected linefeed");
			tc_store_object(tp, tmp, sizeof(tmp));
			tp->restore_state = tc_term;
			tp->state = tc_whitespace;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("val_num: invalid crlf");
			return -1;
		} else if (isdigit(c)) {
			tp->cur_obj.int_val = tp->cur_obj.int_val * 10 + (c - '0');
		} else if (c == ':') {
			tc_store_object(tp, tmp, sizeof(tmp));
			tp->state = tc_val_key;
		} else {
			tc_parse_error("val_num: unexpected character");
			return -1;
		}
		break;
	case tc_val_string:
		if (is_crlf_end(tp, c)) {
			tc_parse_debug("val_string: unexpected linefeed");
			tc_copy_symbol(tp, &tp->cur_obj.str_val);
			tc_store_object(tp, tmp, sizeof(tmp));
			tp->restore_state = tc_term;
			tp->state = tc_whitespace;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("val_string: invalid crlf");
			return -1;
		} else if (c == '^') {
			tp->state = tc_val_ctrl;
		} else if (c == '\\') {
			tp->restore_state = tc_val_string;
			tp->state = tc_escape;
		} else if (c == ':') {
			tc_copy_symbol(tp, &tp->cur_obj.str_val);
			tc_store_object(tp, tmp, sizeof(tmp));
			tp->state = tc_val_key;
		} else if (isdigit(c) && storage_buffer_size(&tp->cur_str) == 0) {
			tp->state = tc_val_delay;
			goto redo;
		} else if (isprint(c) || isspace(c)) {
			storage_buffer_append(&tp->cur_str, &c, 1);
		} else {
			tc_parse_error("val_string: unexpected character");
			return -1;
		}
		break;
	case tc_val_delay:
		if (is_crlf_end(tp, c)) {
			tc_parse_debug("val_delay: unexpected linefeed");
			tc_store_object(tp, tmp, sizeof(tmp));
			tp->restore_state = tc_term;
			tp->state = tc_whitespace;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("val_delay: invalid crlf");
			return -1;
		} else if (isdigit(c)) {
			tp->cur_obj.pad_delay = tp->cur_obj.pad_delay * 10 + (c - '0');
		} else if (c == '*') {
			tp->cur_obj.pad_prop = 1;
			tp->state = tc_val_string;
		} else if (isprint(c) || isspace(c)) {
			tp->state = tc_val_string;
			goto redo;
		} else {
			tc_parse_error("val_delay: unexpected character");
			return -1;
		}
		break;
	case tc_val_ctrl:
		if (is_crlf_end(tp, c)) {
			tc_parse_debug("val_ctrl: unexpected linefeed");
			tc_store_object(tp, tmp, sizeof(tmp));
			tp->restore_state = tc_term;
			tp->state = tc_whitespace;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("val_ctrl: invalid crlf");
			return -1;
		} else if(isprint(c)) {
			c = (c == '?') ? 127 /* DEL */ : (c & 31);
			storage_buffer_append(&tp->cur_str, &c, 1);
			tp->state = tc_val_string;
		} else {
			tc_parse_error("val_ctrl: unexpected character");
			return -1;
		}
		break;
	case tc_escape:
		if (is_crlf_end(tp, c)) {
			tp->state = tp->restore_state;
			return 0;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("escape: invalid crlf");
			return -1;
		}
		switch (c) {
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			tp->cur_oct = (c - '0');
			tp->state = tc_octal;
			break;
		case '\\': c = '\\'; goto append;
		case '^': c = '^'; goto append;
		case ':': c = ':'; goto append;
		case '.': c = '.'; goto append;
		case 'E': case 'e': c = '\033'; goto append; /* \e is undocumented */
		case 'n': case 'l': c = '\n'; goto append;   /* \l is undocumented */
		case 'r': c = '\r'; goto append;
		case 't': c = '\t'; goto append;
		case 'b': c = '\b'; goto append;
		case 'f': c = '\f'; goto append;
		case 'a': c = '\007'; goto append; /* \a is undocumented */
		case 's': c = ' '; goto append;    /* \s is undocumented */
		default:
			tc_parse_debug("escape: illegal character");
			tp->state = tp->restore_state;
			break;
		append:
			storage_buffer_append(&tp->cur_str, &c, 1);
			tp->state = tp->restore_state;
			break;
		}
		break;
	case tc_octal:
		if (is_crlf_end(tp, c)) {
			tp->ln--; /* otherwise is_crlf_end will increment ln twice */
			goto store_octal;
		} else if (is_crlf_error(tp, c)) {
			tc_parse_error("octal: invalid crlf");
			return -1;
		} else if (c >= '0' && c <= '7') {
			tp->cur_oct *= 8;
			tp->cur_oct += (c - '0');
		} else if (isprint(c) || isspace(c)) {
			goto store_octal;
		} else {
			tc_parse_error("octal: unexpected character");
			return -1;
		}
		break;
	store_octal:
		tp->cur_oct &= 0x7f;
		storage_buffer_append(&tp->cur_str, &tp->cur_oct, 1);
		tp->state = tp->restore_state;
		goto redo;
	}
	tp->col++;
	return 0;
}

int tc_init(tc_parser *tp, int debug)
{
	memset(tp, 0, sizeof(tc_parser));
	array_buffer_init(&tp->cap_list, sizeof(tc_cap_list), tc_table_init_size);
	storage_buffer_init(&tp->str_tab, tc_buffer_init_size);
	storage_buffer_init(&tp->cur_str, tc_buffer_init_size);
	lhmap_init_ex(&tp->term_map, tp, sizeof(tc_symbol), sizeof(tc_cap_ref),
		tc_table_init_size, tc_symbol_hash_fn, tc_symbol_compare_fn);
	tp->restore_state = tc_term;
	tp->debug = debug;
	tc_alloc_map(tp);
	return 0;
}

int tc_destroy(tc_parser *tp)
{
	for (size_t i = 0; i < array_buffer_count(&tp->cap_list); i++) {
		tc_cap_list *cap_list = array_buffer_get(&tp->cap_list,
				sizeof(tc_cap_list), i);
		lhmap_destroy(&cap_list->caps);
	}
	array_buffer_destroy(&tp->cap_list);
	storage_buffer_destroy(&tp->str_tab);
	storage_buffer_destroy(&tp->cur_str);
	lhmap_destroy(&tp->term_map);
	return 0;
}

int tc_read(tc_parser *tp, const char *filesymbol)
{
	FILE *f;
	char buf[4096];
	size_t n;

	if ((f = fopen(filesymbol, "r")) == NULL) {
		tc_error("error opening %s: %s\n", filesymbol, strerror(errno));
		return -1;
	}

	tp->ln = 1, tp->col = 0;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
		for (char *p = buf; p != buf + n; p++) {
			if (tc_parse(tp, *p) < 0) goto error;
		}
	}

	fclose(f);
	return 0;
error:
	fclose(f);
	return -1;
}

void tc_dump_stats(tc_parser *tp)
{
	size_t cap_map_size, cap_list_size, term_map_size, sym_tab_size;

	cap_list_size = array_buffer_size(&tp->cap_list, sizeof(tc_cap_list));
	cap_map_size = 0;
	for (size_t i = 0; i < array_buffer_count(&tp->cap_list); i++) {
		tc_cap_list *cap_list = array_buffer_get(&tp->cap_list,
				sizeof(tc_cap_list), i);
		cap_map_size += lhmap_size(&cap_list->caps);
	}
	term_map_size =  lhmap_size(&tp->term_map);
	sym_tab_size = storage_buffer_size(&tp->str_tab);

	tc_verbose("# data");
	tc_verbose("| aliases  | %7zu |", lhmap_count(&tp->term_map));
	tc_verbose("| entries  | %7zu |", array_buffer_count(&tp->cap_list));
	tc_verbose("# memory");
	tc_verbose("| cap_list | %7zu |", cap_list_size);
	tc_verbose("| cap_map  | %7zu |", cap_map_size);
	tc_verbose("| term_map | %7zu |", term_map_size);
	tc_verbose("| str_tab  | %7zu |", sym_tab_size);
}
