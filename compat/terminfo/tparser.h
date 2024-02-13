#pragma once

#include "buffer.h"
#include "hashmap.h"

typedef struct tc_symbol tc_symbol;
typedef enum tc_cap_type tc_cap_type;
typedef struct tc_cap_obj tc_cap_obj;
typedef struct tc_cap_ref tc_cap_ref;
typedef struct tc_cap_list tc_cap_list;
typedef enum tc_state tc_state;
typedef struct tc_parser tc_parser;

enum {
	tc_table_init_size = 8,
	tc_buffer_init_size = 32,
};

struct tc_symbol
{
	int offset;
	int length;
};

enum tc_cap_type
{
	tc_cap_type_boolean,
	tc_cap_type_number,
	tc_cap_type_string,
};

struct tc_cap_obj
{
	tc_symbol str_key;
	tc_symbol str_val;
	char type;
	char pad_prop;
	short pad_delay;
	int int_val;
};

struct tc_cap_ref
{
	int cap_idx;
};

struct tc_cap_list
{
	lhmap caps;
};

enum tc_state
{
	tc_whitespace,
	tc_comment,
	tc_term,
	tc_val_key,
	tc_val_skip,
	tc_val_num,
	tc_val_string,
	tc_val_delay,
	tc_val_ctrl,
	tc_escape,
	tc_octal,
};

struct tc_parser
{
	lhmap term_map;
	array_buffer cap_list;
	storage_buffer str_tab;
	tc_state state;
	tc_state restore_state;
	storage_buffer cur_str;
	tc_symbol cur_term;
	tc_cap_ref cur_cap;
	tc_cap_obj cur_obj;
	char cur_oct;
	int crlf, debug;
	int ln, col;
};

char* tc_tinfo_strval(const char *val);

size_t tc_symbol_hash_fn(lhmap *h, void *key);
int tc_symbol_compare_fn(lhmap *h, void *key1, void *key2);
char* tc_get_string(tc_parser *tp, tc_symbol sym);
tc_symbol tc_make_symbol(tc_parser *tp, const char *str, size_t len);

size_t tc_sym_str(tc_parser *tp, char *tmp, size_t n, tc_symbol sym);
int tc_obj_str(tc_parser *tp, char *tmp, size_t n, tc_cap_obj *obj);

int tc_init(tc_parser *tp, int debug);
int tc_destroy(tc_parser *tp);
int tc_read(tc_parser *tp, const char *filesymbol);
void tc_dump_stats(tc_parser *tp);
