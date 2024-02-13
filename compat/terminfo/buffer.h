/*
 * buffer functions
 *
 * these functions do not use realloc and realloc is not present in this
 * library because it is unsafe in multi-threaded programs. these functions
 * are designed so that array bounds are tied to a unique pointer identity.
 * this allows us to perform ordered stores expanding and shrinking bounds,
 * writing the data pointer first when expanding bounds, or writing bounds
 * first when shrinking bounds, to ensure that other threads will always have
 * valid bounds that fit within the allocation. these functions still need to
 * be modified to correctly use atomic_load_acquire and atomic_store_release.
*/

#pragma once

#include "stddef.h"
#include "stdlib.h"
#include "string.h"

typedef struct array_buffer array_buffer;
typedef struct storage_buffer storage_buffer;

typedef unsigned long long ullong;

/*
 * bitmanip functions
 */

#if defined (_MSC_VER)
#include <intrin.h>
#endif

#if defined (__GNUC__)
static inline unsigned clz(ullong val)
{
    return val == 0 ? 64 : __builtin_clzll(val);
}
#elif defined (_MSC_VER)
static inline unsigned clz(ullong val)
{
    return (unsigned)_lzcnt_u64(val);
}
#endif

static inline ullong rupgtpow2(ullong x)
{
    return 1ull << (64 - clz(x-1));
}

/*
 * array buffer
 *
 * array buffer has a capacity and count to store fixed stride data in
 * a resizable buffer. stride is a parameter because the functions are
 * designed to be used in macros that contains sizeof(T) expression.
 */

struct array_buffer
{
    size_t capacity;
    size_t count;
    char *data;
};

static void array_buffer_init(array_buffer *ab, size_t stride, size_t capacity)
{
    ab->capacity = capacity;
    ab->count = 0;
    ab->data = (char*)malloc(stride * capacity);
    memset(ab->data, 0, stride * capacity);
}

static void array_buffer_destroy(array_buffer *ab)
{
    free(ab->data);
    ab->data = NULL;
}

static size_t array_buffer_count(array_buffer *ab)
{
    return ab->count;
}

static size_t array_buffer_size(array_buffer *ab, size_t stride)
{
    return ab->count * stride;
}

static size_t array_buffer_capacity(array_buffer *ab, size_t stride)
{
    return ab->capacity * stride;
}

static void* array_buffer_get(array_buffer *ab, size_t stride, size_t idx)
{
    return ab->data + idx * stride;
}

static void array_buffer_resize(array_buffer *ab, size_t stride, size_t count)
{
    size_t old_capacity = ab->capacity, new_capacity = rupgtpow2(count);
    char * data = (char*)malloc(stride * new_capacity), *old_data = ab->data;
    if (new_capacity >= old_capacity) {
        memcpy(data, ab->data, stride * old_capacity);
        memset(data + stride * old_capacity, 0, stride * (new_capacity - old_capacity));
        /* write pointer first when expanding: todo: use atomic_store_release */
        ab->data = data;
        ab->capacity = new_capacity;
    } else {
        memcpy(data, ab->data, stride * new_capacity);
        /* write bound first when shrinking: todo: use atomic_store_release */
        ab->capacity = new_capacity;
        ab->data = data;
    }
    free(old_data);
}

static size_t array_buffer_alloc(array_buffer *ab, size_t stride, size_t count)
{
    if (ab->count + count > ab->capacity) {
        array_buffer_resize(ab, stride, ab->count + count);
    }
    size_t idx = ab->count;
    ab->count += count;
    return idx;
}

static size_t array_buffer_add(array_buffer *ab, size_t stride, void *ptr)
{
    size_t idx = array_buffer_alloc(ab, stride, 1);
    memcpy(ab->data + (idx * stride), ptr, stride);
    return idx;
}

/*
 * storage buffer
 *
 * storage buffer holds a capacity and an offset to provide for allocation
 * and storage of aligned variable length data in a resizable buffer.
 */

struct storage_buffer
{
    size_t capacity;
    size_t offset;
    char *data;
};

static void storage_buffer_init(storage_buffer *sb, size_t capacity)
{
    sb->capacity = capacity;
    sb->offset = 0;
    sb->data = (char*)malloc(sb->capacity);
    memset(sb->data, 0, sb->capacity);
}

static void storage_buffer_destroy(storage_buffer *sb)
{
    free(sb->data);
    sb->data = NULL;
}

static size_t storage_buffer_size(storage_buffer *sb)
{
    return sb->offset;
}

static size_t storage_buffer_capacity(storage_buffer *sb)
{
    return sb->capacity;
}

static void* storage_buffer_get(storage_buffer *sb, size_t idx)
{
    return sb->data + idx;
}

static void storage_buffer_reset(storage_buffer *sb)
{
    sb->offset = 0;
}

static void storage_buffer_resize(storage_buffer *sb, size_t offset)
{
    size_t old_capacity = sb->capacity, new_capacity = rupgtpow2(offset);
    char *data = (char*)malloc(new_capacity), *old_data = sb->data;
    if (new_capacity >= old_capacity) {
        memcpy(data, sb->data, old_capacity);
        memset(data + old_capacity, 0, new_capacity - old_capacity);
        /* write pointer first when expanding: todo: use atomic_store_release */
        sb->data = data;
        sb->capacity = new_capacity;
    } else {
        memcpy(data, sb->data, new_capacity);
        /* write bound first when shrinking: todo: use atomic_store_release */
        sb->capacity = new_capacity;
        sb->data = data;
    }
    sb->offset = offset;
    free(old_data);
}

static size_t storage_buffer_alloc(storage_buffer *sb, size_t size, size_t align)
{
    size_t offset = sb->offset, max_align = align > 8 ? 8 : align;
    size_t our_offset = (offset + max_align - 1) & ~(max_align - 1);
    size_t align_size = (size   + max_align - 1) & ~(max_align - 1);
    if (our_offset + align_size > sb->capacity) {
        storage_buffer_resize(sb, our_offset + align_size);
    }
    sb->offset = our_offset + align_size;
    return our_offset;
}

static size_t storage_buffer_append(storage_buffer *sb, const char *data, size_t len)
{
    size_t o = storage_buffer_alloc(sb, len, 1);
    memcpy(storage_buffer_get(sb, o), data, len);
    return o;
}
