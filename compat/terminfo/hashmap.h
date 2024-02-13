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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
 * hmap hash table interface
 */

typedef struct hmap hmap;
typedef struct hmap_iter hmap_iter;

typedef size_t (*hmap_hash_fn)(hmap *h, void *key);
typedef int (*hmap_compare_fn)(hmap *h, void *key1, void *key2);

struct hmap_iter { hmap *h; size_t idx; };

static inline size_t hmap_stride(hmap *h);
static inline hmap_iter hmap_iter_next(hmap_iter iter);
static inline void* hmap_iter_key(hmap_iter iter);
static inline void* hmap_iter_val(hmap_iter iter);
static inline int hmap_iter_eq(hmap_iter iter1, hmap_iter iter2);
static inline int hmap_iter_neq(hmap_iter iter1, hmap_iter iter2);
static inline hmap_iter hmap_iter_begin(hmap *h);
static inline hmap_iter hmap_iter_end(hmap *h);
static inline void* hmap_userdata(hmap *h);
static inline size_t hmap_size(hmap *h);
static inline size_t hmap_count(hmap *h);
static inline size_t hmap_capacity(hmap *h);
static inline size_t hmap_load(hmap *h);
static inline void hmap_init(hmap *h,
    size_t key_size, size_t val_size, size_t limit);
static inline void hmap_init_ex(hmap *h, void *userdata,
    size_t key_size, size_t val_size, size_t limit,
    hmap_hash_fn hasher, hmap_compare_fn compare);
static inline void hmap_destroy(hmap *h);
static inline void hmap_clear(hmap *h);
static inline hmap_iter hmap_insert(hmap *h, void *key, void *val);
static inline void* hmap_get(hmap *h, void *key);
static inline hmap_iter hmap_find(hmap *h, void *key);
static inline void hmap_erase(hmap *h, void *key);

/*
 * lhmap linked hash table interface
 */

typedef struct lhmap lhmap;
typedef struct lhmap_iter lhmap_iter;

typedef size_t (*lhmap_hash_fn)(lhmap *h, void *key);
typedef int (*lhmap_compare_fn)(lhmap *h, void *key1, void *key2);

struct lhmap_iter { lhmap *h; size_t idx; };

static inline size_t lhmap_stride(lhmap *h);
static inline lhmap_iter lhmap_iter_next(lhmap_iter iter);
static inline void* lhmap_iter_key(lhmap_iter iter);
static inline void* lhmap_iter_val(lhmap_iter iter);
static inline int lhmap_iter_eq(lhmap_iter iter1, lhmap_iter iter2);
static inline int lhmap_iter_neq(lhmap_iter iter1, lhmap_iter iter2);
static inline lhmap_iter lhmap_iter_begin(lhmap *h);
static inline lhmap_iter lhmap_iter_end(lhmap *h);
static inline void* lhmap_userdata(lhmap *h);
static inline size_t lhmap_size(lhmap *h);
static inline size_t lhmap_count(lhmap *h);
static inline size_t lhmap_capacity(lhmap *h);
static inline size_t lhmap_load(lhmap *h);
static inline void lhmap_init(lhmap *h,
    size_t key_size, size_t val_size, size_t limit);
static inline void lhmap_init_ex(lhmap *h, void *userdata,
    size_t key_size, size_t val_size, size_t limit,
    lhmap_hash_fn hasher, lhmap_compare_fn compare);
static inline void lhmap_destroy(lhmap *h);
static inline void lhmap_clear(lhmap *h);
static inline lhmap_iter lhmap_insert(lhmap *h,
    lhmap_iter iter, void *key, void *val);
static inline void* lhmap_get(lhmap *h, void *key);
static inline lhmap_iter lhmap_find(lhmap *h, void *key);
static inline void lhmap_erase(lhmap *h, void *key);

/*
 * hmap common
 */

typedef enum hmap_bitmap_state hmap_bitmap_state;
enum hmap_bitmap_state {
    hmap_available = 0,
    hmap_occupied = 1,
    hmap_deleted = 2,
    hmap_recycled = 3
};

static const size_t hmap_default_size =    (2<<3);  /* 16 */
static const size_t hmap_load_factor =     (2<<15); /* 0.5 */
static const size_t hmap_load_multiplier = (2<<16); /* 1.0 */
static const size_t hmap_empty_offset = (size_t)-1LL;

static inline size_t hmap_bitmap_size(size_t limit)
{
    return (((limit + 3) >> 2) + 7) & ~7;
}

static inline size_t hmap_bitmap_idx(size_t i)
{
    return i >> 5;
}

static inline size_t hmap_bitmap_shift(size_t i)
{
    return ((i << 1) & 63);
}

static inline hmap_bitmap_state hmap_bitmap_get(uint64_t *bitmap, size_t i)
{
    return (hmap_bitmap_state)((bitmap[hmap_bitmap_idx(i)] >> hmap_bitmap_shift(i)) & 3);
}

static inline void hmap_bitmap_set(uint64_t *bitmap, size_t i, uint64_t value)
{
    bitmap[hmap_bitmap_idx(i)] |= (value << hmap_bitmap_shift(i));
}

static inline void hmap_bitmap_clear(uint64_t *bitmap, size_t i, uint64_t value)
{
    bitmap[hmap_bitmap_idx(i)] &= ~(value << hmap_bitmap_shift(i));
}

static inline int hmap_ispow2(size_t v) { return v && !(v & (v-1)); }

/*
 * hmap hash table implementation
 */

struct hmap
{
    size_t key_size;
    size_t val_size;
    size_t used;
    size_t tombs;
    size_t limit;
    hmap_hash_fn hasher;
    hmap_compare_fn compare;
    unsigned char *data;
    uint64_t *bitmap;
    void *userdata;
};

static inline size_t hmap_default_hash_fn(hmap *h, void *key)
{
    size_t k = 0;
    memcpy(&k, key, h->key_size < sizeof(k) ? h->key_size : sizeof(k));
    return k;
}

static inline int hmap_default_compare_fn(hmap *h, void *key1, void *key2)
{
    return memcmp(key1, key2, h->key_size) == 0;
}

static inline size_t hmap_stride(hmap *h)
{
    return h->key_size + h->val_size;
}

static inline void* hmap_data_key(hmap *h, size_t idx)
{
    return h->data + idx * hmap_stride(h);
}

static inline void* hmap_data_val(hmap *h, size_t idx)
{
    return h->data + h->key_size + idx * hmap_stride(h);
}

static inline size_t hmap_iter_step(hmap *h, size_t idx)
{
    while (idx < h->limit && (hmap_bitmap_get(h->bitmap,
        idx) & hmap_occupied) != hmap_occupied) idx++;
    return idx;
}

static inline hmap_iter hmap_iter_make(hmap *h, size_t idx)
{
    hmap_iter iter = { h, idx }; return iter;
}

static inline hmap_iter hmap_iter_next(hmap_iter iter)
{
    return hmap_iter_make(iter.h, hmap_iter_step(iter.h, iter.idx + 1));
}

static inline void* hmap_iter_key(hmap_iter iter)
{
    return hmap_data_key(iter.h, hmap_iter_step(iter.h, iter.idx));
}

static inline void* hmap_iter_val(hmap_iter iter)
{
    return hmap_data_val(iter.h, hmap_iter_step(iter.h, iter.idx));
}

static inline int hmap_iter_eq(hmap_iter iter1, hmap_iter iter2)
{
    size_t i1 = hmap_iter_step(iter1.h, iter1.idx);
    size_t i2 = hmap_iter_step(iter2.h, iter2.idx);
    return iter1.h == iter2.h && i1 == i2;
}

static inline int hmap_iter_neq(hmap_iter iter1, hmap_iter iter2)
{
    size_t i1 = hmap_iter_step(iter1.h, iter1.idx);
    size_t i2 = hmap_iter_step(iter2.h, iter2.idx);
    return iter1.h != iter2.h || i1 != i2;
}

static inline hmap_iter hmap_iter_begin(hmap *h)
{
    return hmap_iter_make(h, hmap_iter_step(h, 0));
}

static inline hmap_iter hmap_iter_end(hmap *h)
{
    return hmap_iter_make(h, h->limit);
}

static inline void* hmap_userdata(hmap *h)
{
    return h->userdata;
}

static inline size_t hmap_size(hmap *h)
{
    return h->used * (h->key_size + h->val_size);
}

static inline size_t hmap_count(hmap *h)
{
    return h->used;
}

static inline size_t hmap_capacity(hmap *h)
{
    return h->limit;
}

static inline size_t hmap_load(hmap *h)
{
    return (h->used + h->tombs) * hmap_load_multiplier / h->limit;
}

static inline size_t hmap_index_mask(hmap *h)
{
    return h->limit - 1;
}

static inline size_t hmap_hash_index(hmap *h, size_t i)
{
    return i & hmap_index_mask(h);
}

static inline size_t hmap_key_index(hmap *h, void *key)
{
    return hmap_hash_index(h, h->hasher(h, key));
}

static inline void hmap_init_ex(hmap *h, void *userdata,
    size_t key_size, size_t val_size, size_t limit,
    hmap_hash_fn hasher, hmap_compare_fn compare)
{
    size_t stride = key_size + val_size;
    size_t data_size = stride * limit;
    size_t bitmap_size = hmap_bitmap_size(limit);
    size_t total_size = data_size + bitmap_size;

    assert(hmap_ispow2(limit));

    h->key_size = key_size;
    h->val_size = val_size;
    h->used = 0;
    h->tombs = 0;
    h->limit = limit;
    h->hasher = hasher;
    h->compare = compare;
    h->data = (unsigned char*)malloc(total_size);
    h->bitmap = (uint64_t*)(h->data + data_size);
    h->userdata = userdata;

    memset(h->data, 0, total_size);
}

static inline void hmap_init(hmap *h,
    size_t key_size, size_t val_size, size_t limit)
{
    hmap_init_ex(h, NULL, key_size, val_size, limit,
        hmap_default_hash_fn, hmap_default_compare_fn);
}

static inline void hmap_destroy(hmap *h)
{
    free(h->data);
    h->data = NULL;
    h->bitmap = NULL;
}

static inline void hmap_resize_internal(hmap *h,
    unsigned char *old_data, uint64_t *old_bitmap, size_t old_limit, size_t new_limit)
{
    size_t stride = h->key_size + h->val_size;
    size_t data_size = stride * new_limit;
    size_t bitmap_size = hmap_bitmap_size(new_limit);
    size_t total_size = data_size + bitmap_size;

    assert(hmap_ispow2(new_limit));

    h->data = (unsigned char*)malloc(total_size);
    h->bitmap = (uint64_t*)((char*)h->data + data_size);
    h->limit = new_limit;
    memset(h->bitmap, 0, bitmap_size);

    size_t i = 0;
    for (unsigned char *k = old_data; k != old_data + old_limit * stride; k += stride, i++) {
        if ((hmap_bitmap_get(old_bitmap, i) & hmap_occupied) != hmap_occupied) continue;
        for (size_t j = hmap_key_index(h, k); ; j = (j+1) & hmap_index_mask(h)) {
            if ((hmap_bitmap_get(h->bitmap, j) & hmap_occupied) != hmap_occupied) {
                hmap_bitmap_set(h->bitmap, j, hmap_occupied);
                memcpy(hmap_data_key(h, j), k, stride);
                break;
            }
        }
    }

    h->tombs = 0;
    free(old_data);
}

static inline void hmap_clear(hmap *h)
{
    size_t bitmap_size = hmap_bitmap_size(h->limit);
    memset(h->bitmap, 0, bitmap_size);
    h->used = h->tombs = 0;
}

static inline hmap_iter hmap_insert(hmap *h, void *key, void *val)
{
    for (size_t i = hmap_key_index(h, key); ; i = (i+1) & hmap_index_mask(h)) {
        hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
        if ((state & hmap_occupied) != hmap_occupied) {
            hmap_bitmap_set(h->bitmap, i, hmap_occupied);
            memcpy(hmap_data_key(h, i), key, h->key_size);
            memcpy(hmap_data_val(h, i), val, h->val_size);
            h->used++;
            if ((state & hmap_deleted) == hmap_deleted) h->tombs--;
            if (hmap_load(h) > hmap_load_factor) {
                hmap_resize_internal(h, h->data, h->bitmap, h->limit, h->limit << 1);
                for (i = hmap_key_index(h, key); ; i = (i+1) & hmap_index_mask(h)) {
                    hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
                         if (state == hmap_available) abort();
                    else if (state == hmap_deleted); /* skip */
                    else if (h->compare(h, hmap_data_key(h, i), key)) {
                        hmap_iter iter = { h, i };
                        return iter;
                    }
                }
            } else {
                hmap_iter iter = { h, i };
                return iter;
            }
        } else if (h->compare(h, hmap_data_key(h, i), key)) {
            memcpy(hmap_data_val(h, i), val, h->val_size);
            hmap_iter iter = { h, i };
            return iter;
        }
    }

    return hmap_iter_end(h);
}

static inline void* hmap_get(hmap *h, void *key)
{
    for (size_t i = hmap_key_index(h, key); ; i = (i+1) & hmap_index_mask(h)) {
        hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
        if ((state & hmap_occupied) != hmap_occupied) {
            hmap_bitmap_set(h->bitmap, i, hmap_occupied);
            memcpy(hmap_data_key(h, i), key, h->key_size);
            h->used++;
            if ((state & hmap_deleted) == hmap_deleted) h->tombs--;
            if (hmap_load(h) > hmap_load_factor) {
                hmap_resize_internal(h, h->data, h->bitmap, h->limit, h->limit << 1);
                for (i = hmap_key_index(h, key); ; i = (i+1) & hmap_index_mask(h)) {
                    hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
                         if (state == hmap_available) abort();
                    else if (state == hmap_deleted); /* skip */
                    else if (h->compare(h, hmap_data_key(h, i), key)) {
                        return hmap_data_val(h, i);
                    }
                }
            }
            return hmap_data_val(h, i);
        } else if (h->compare(h, hmap_data_key(h, i), key)) {
            return hmap_data_val(h, i);
        }
    }
}

static inline hmap_iter hmap_find(hmap *h, void *key)
{
    for (size_t i = hmap_key_index(h, key); ; i = (i+1) & hmap_index_mask(h)) {
        hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
             if (state == hmap_available)           /* notfound */ break;
        else if (state == hmap_deleted);            /* skip */
        else if (h->compare(h, hmap_data_key(h, i), key)) {
            return hmap_iter_make(h, i);
        }
    }
    return hmap_iter_end(h);
}

static inline void hmap_erase(hmap *h, void *key)
{
    for (size_t i = hmap_key_index(h, key); ; i = (i+1) & hmap_index_mask(h)) {
        hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
             if (state == hmap_available)           /* notfound */ break;
        else if (state == hmap_deleted);            /* skip */
        else if (h->compare(h, hmap_data_key(h, i), key)) {
            hmap_bitmap_set(h->bitmap, i, hmap_deleted);
            hmap_bitmap_clear(h->bitmap, i, hmap_occupied);
            h->used--;
            h->tombs++;
            return;
        }
    }
}

/*
 * lhmap linked hash table implementation
 */

struct lhmap
{
    size_t key_size;
    size_t val_size;
    size_t used;
    size_t tombs;
    size_t limit;
    lhmap_hash_fn hasher;
    lhmap_compare_fn compare;
    unsigned char *data;
    uint64_t *bitmap;
    size_t head;
    size_t tail;
    void *userdata;
};

typedef struct lhmap_link lhmap_link;

struct lhmap_link
{
    size_t prev;
    size_t next;
};

static inline size_t lhmap_default_hash_fn(lhmap *h, void *key)
{
    size_t k = 0;
    memcpy(&k, key, h->key_size < sizeof(k) ? h->key_size : sizeof(k));
    return k;
}

static inline int lhmap_default_compare_fn(lhmap *h, void *key1, void *key2)
{
    return memcmp(key1, key2, h->key_size) == 0;
}

static inline size_t lhmap_stride(lhmap *h)
{
    return sizeof(lhmap_link) + h->key_size + h->val_size;
}

static inline lhmap_link* lhmap_old_data_link(lhmap *h, unsigned char *old_data, size_t idx)
{
    return (lhmap_link*)(old_data + idx * lhmap_stride(h));
}

static inline void* lhmap_old_data_key(lhmap *h, unsigned char *old_data, size_t idx)
{
    return old_data + sizeof(lhmap_link) + idx * lhmap_stride(h);
}

static inline lhmap_link* lhmap_data_link(lhmap *h, size_t idx)
{
    return (lhmap_link*)(h->data + idx * lhmap_stride(h));
}

static inline void* lhmap_data_key(lhmap *h, size_t idx)
{
    return h->data + sizeof(lhmap_link) + idx * lhmap_stride(h);
}

static inline void* lhmap_data_val(lhmap *h, size_t idx)
{
    return h->data + sizeof(lhmap_link) + h->key_size + idx * lhmap_stride(h);
}

static inline lhmap_iter lhmap_iter_make(lhmap *h, size_t idx)
{
    lhmap_iter iter = { h, idx }; return iter;
}

static inline lhmap_iter lhmap_iter_next(lhmap_iter iter)
{
    return lhmap_iter_make(iter.h, lhmap_data_link(iter.h, iter.idx)->next);
}

static inline void* lhmap_iter_link(lhmap_iter iter)
{
    return lhmap_data_link(iter.h, iter.idx);
}

static inline void* lhmap_iter_key(lhmap_iter iter)
{
    return lhmap_data_key(iter.h, iter.idx);
}

static inline void* lhmap_iter_val(lhmap_iter iter)
{
    return lhmap_data_val(iter.h, iter.idx);
}

static inline int lhmap_iter_eq(lhmap_iter iter1, lhmap_iter iter2)
{
    return iter1.h == iter2.h && iter1.idx == iter2.idx;
}

static inline int lhmap_iter_neq(lhmap_iter iter1, lhmap_iter iter2)
{
    return iter1.h != iter2.h || iter1.idx != iter2.idx;
}

static inline lhmap_iter lhmap_iter_begin(lhmap *h)
{
    return lhmap_iter_make( h, h->head);
}

static inline lhmap_iter lhmap_iter_end(lhmap *h)
{
    return lhmap_iter_make(h, hmap_empty_offset);
}

static inline void* lhmap_userdata(lhmap *h)
{
    return h->userdata;
}

static inline size_t lhmap_size(lhmap *h)
{
    return h->used * (sizeof(lhmap_link) + h->key_size + h->val_size);
}

static inline size_t lhmap_count(lhmap *h)
{
    return h->used;
}

static inline size_t lhmap_capacity(lhmap *h)
{
    return h->limit;
}

static inline size_t lhmap_load(lhmap *h)
{
    return (h->used + h->tombs) * hmap_load_multiplier / h->limit;
}

static inline size_t lhmap_index_mask(lhmap *h)
{
    return h->limit - 1;
}

static inline size_t lhmap_hash_index(lhmap *h, size_t i)
{
    return i & lhmap_index_mask(h);
}

static inline size_t lhmap_key_index(lhmap *h, void *key)
{
    return lhmap_hash_index(h, h->hasher(h, key));
}

static inline void lhmap_init_ex(lhmap *h, void *userdata,
    size_t key_size, size_t val_size, size_t limit,
    lhmap_hash_fn hasher, lhmap_compare_fn compare)
{
    size_t stride = sizeof(lhmap_link) + key_size + val_size;
    size_t data_size = stride * limit;
    size_t bitmap_size = hmap_bitmap_size(limit);
    size_t total_size = data_size + bitmap_size;

    assert(hmap_ispow2(limit));

    h->key_size = key_size;
    h->val_size = val_size;
    h->used = 0;
    h->tombs = 0;
    h->limit = limit;
    h->hasher = hasher;
    h->compare = compare;
    h->data = (unsigned char*)malloc(total_size);
    h->bitmap = (uint64_t*)(h->data + data_size);
    h->head = hmap_empty_offset;
    h->tail = hmap_empty_offset;
    h->userdata = userdata;

    memset(h->data, 0, total_size);
}

static inline void lhmap_init(lhmap *h,
    size_t key_size, size_t val_size, size_t limit)
{
    lhmap_init_ex(h, NULL, key_size, val_size, limit,
        lhmap_default_hash_fn, lhmap_default_compare_fn);
}

static inline void lhmap_destroy(lhmap *h)
{
    free(h->data);
    h->data = NULL;
    h->bitmap = NULL;
}

static inline void lhmap_resize_internal(lhmap *h,
    unsigned char *old_data, uint64_t *old_bitmap, size_t old_limit, size_t new_limit)
{
    size_t stride = sizeof(lhmap_link) + h->key_size + h->val_size;
    size_t data_size = stride * new_limit;
    size_t bitmap_size = hmap_bitmap_size(new_limit);
    size_t total_size = data_size + bitmap_size;

    assert(hmap_ispow2(new_limit));

    h->data = (unsigned char*)malloc(total_size);
    h->bitmap = (uint64_t*)((char*)h->data + data_size);
    h->limit = new_limit;
    memset(h->bitmap, 0, bitmap_size);

    size_t k = hmap_empty_offset;
    for (size_t i = h->head; i != hmap_empty_offset;
         i = lhmap_old_data_link(h, old_data, i)->next)
    {
        void *key = lhmap_old_data_key(h, old_data, i);
        for (size_t j = lhmap_key_index(h, key); ; j = (j+1) & lhmap_index_mask(h))
        {
            if ((hmap_bitmap_get(h->bitmap, j) & hmap_occupied) != hmap_occupied) {
                hmap_bitmap_set(h->bitmap, j, hmap_occupied);
                if (i == h->head) h->head = j;
                if (i == h->tail) h->tail = j;
                memcpy(lhmap_data_key(h, j), key, h->key_size + h->val_size);
                lhmap_data_link(h, j)->next = hmap_empty_offset;
                if (k == hmap_empty_offset) {
                    lhmap_data_link(h, j)->prev = hmap_empty_offset;
                } else {
                    lhmap_data_link(h, j)->prev = k;
                    lhmap_data_link(h, k)->next = j;
                }
                k = j;
                break;
            }
        }
    }

    h->tombs = 0;
    free(old_data);
}

static inline void lhmap_clear(lhmap *h)
{
    size_t bitmap_size = hmap_bitmap_size(h->limit);
    memset(h->bitmap, 0, bitmap_size);
    h->head = h->tail = hmap_empty_offset;
    h->used = h->tombs = 0;
}

/* inserts indice link before specified position */
static inline void lhmap_insert_link_internal(lhmap *h, size_t pos, size_t i)
{
    if (h->head == h->tail && h->head == hmap_empty_offset) {
        h->head = h->tail = i;
        lhmap_data_link(h, i)->next = hmap_empty_offset;
        lhmap_data_link(h, i)->prev = hmap_empty_offset;
    } else if (pos == hmap_empty_offset) {
        lhmap_data_link(h, i)->next = hmap_empty_offset;
        lhmap_data_link(h, i)->prev = h->tail;
        lhmap_data_link(h, h->tail)->next = i;
        h->tail = i;
    } else {
        lhmap_data_link(h, i)->next = pos;
        lhmap_data_link(h, i)->prev = lhmap_data_link(h, pos)->prev;
        if (lhmap_data_link(h, pos)->prev != hmap_empty_offset) {
            lhmap_data_link(h, lhmap_data_link(h, pos)->prev)->next = i;
        }
        lhmap_data_link(h, pos)->prev = i;
        if (h->head == pos) h->head = i;
    }
}

/* remove indice link at the specified index */
static inline void lhmap_erase_link_internal(lhmap *h, size_t i)
{
    assert(h->head != hmap_empty_offset && h->tail != hmap_empty_offset);
    if (h->head == h->tail && i == h->head) {
        h->head = h->tail = hmap_empty_offset;
    } else {
        if (h->head == i) h->head = lhmap_data_link(h, i)->next;
        if (h->tail == i) h->tail = lhmap_data_link(h, i)->prev;
        if (lhmap_data_link(h, i)->prev != hmap_empty_offset) {
            lhmap_data_link(h, lhmap_data_link(h, i)->prev)->next =
                lhmap_data_link(h, i)->next;
        }
        if (lhmap_data_link(h, i)->next != hmap_empty_offset) {
            lhmap_data_link(h, lhmap_data_link(h, i)->next)->prev =
                lhmap_data_link(h, i)->prev;
        }
    }
}

static inline lhmap_iter lhmap_insert(lhmap *h,
    lhmap_iter iter, void *key, void *val)
{
    for (size_t i = lhmap_key_index(h, key); ; i = (i+1) & lhmap_index_mask(h)) {
        hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
        if ((state & hmap_occupied) != hmap_occupied) {
            hmap_bitmap_set(h->bitmap, i, hmap_occupied);
            memcpy(lhmap_data_key(h, i), key, h->key_size);
            memcpy(lhmap_data_val(h, i), val, h->val_size);
            lhmap_insert_link_internal(h, iter.idx, i);
            h->used++;
            if ((state & hmap_deleted) == hmap_deleted) h->tombs--;
            if (lhmap_load(h) > hmap_load_factor) {
                lhmap_resize_internal(h, h->data, h->bitmap, h->limit, h->limit << 1);
                for (i = lhmap_key_index(h, key); ; i = (i+1) & lhmap_index_mask(h)) {
                    hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
                         if (state == hmap_available) abort();
                    else if (state == hmap_deleted); /* skip */
                    else if (h->compare(h, lhmap_data_key(h, i), key)) {
                        return lhmap_iter_make(h, i);
                    }
                }
            } else {
                return lhmap_iter_make(h, i);
            }
        } else if (h->compare(h, lhmap_data_key(h, i), key)) {
            memcpy(lhmap_data_val(h, i), val, h->val_size);
            return lhmap_iter_make(h, i);
        }
    }
}

static inline void* lhmap_get(lhmap *h, void *key)
{
    for (size_t i = lhmap_key_index(h, key); ; i = (i+1) & lhmap_index_mask(h)) {
        hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
        if ((state & hmap_occupied) != hmap_occupied) {
            hmap_bitmap_set(h->bitmap, i, hmap_occupied);
            memcpy(lhmap_data_key(h, i), key, h->key_size);
            lhmap_insert_link_internal(h, hmap_empty_offset, i);
            h->used++;
            if ((state & hmap_deleted) == hmap_deleted) h->tombs--;
            if (lhmap_load(h) > hmap_load_factor) {
                lhmap_resize_internal(h, h->data, h->bitmap, h->limit, h->limit << 1);
                for (i = lhmap_key_index(h, key); ; i = (i+1) & lhmap_index_mask(h)) {
                    hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
                         if (state == hmap_available) abort();
                    else if (state == hmap_deleted); /* skip */
                    else if (h->compare(h, lhmap_data_key(h, i), key)) {
                        return lhmap_data_val(h, i);
                    }
                }
            }
            return lhmap_data_val(h, i);
        } else if (h->compare(h, lhmap_data_key(h, i), key)) {
            return lhmap_data_val(h, i);
        }
    }
}

static inline lhmap_iter lhmap_find(lhmap *h, void *key)
{
    for (size_t i = lhmap_key_index(h, key); ; i = (i+1) & lhmap_index_mask(h)) {
        hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
             if (state == hmap_available)           /* notfound */ break;
        else if (state == hmap_deleted);            /* skip */
        else if (h->compare(h, lhmap_data_key(h, i), key)) {
            return lhmap_iter_make(h, i);
        }
    }
    return lhmap_iter_end(h);
}

static inline void lhmap_erase(lhmap *h, void *key)
{
    for (size_t i = lhmap_key_index(h, key); ; i = (i+1) & lhmap_index_mask(h)) {
        hmap_bitmap_state state = hmap_bitmap_get(h->bitmap, i);
             if (state == hmap_available)           /* notfound */ break;
        else if (state == hmap_deleted);            /* skip */
        else if (h->compare(h, lhmap_data_key(h, i), key)) {
            hmap_bitmap_set(h->bitmap, i, hmap_deleted);
            hmap_bitmap_clear(h->bitmap, i, hmap_occupied);
            lhmap_erase_link_internal(h, i);
            h->used--;
            h->tombs++;
            return;
        }
    }
}
