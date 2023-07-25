#pragma once

#include <stddef.h>

// Info about one bucket of memory managed by a mempool
struct MEMPOOL_BUCKET
{
    struct MEMPOOL_BUCKET* next;
    size_t used;
    size_t size;
    char memory[];
};

// A memory pool
struct MEMPOOL
{
    size_t bucket_size;
    struct MEMPOOL_BUCKET* first;
};

// Initialize a new memory pool
void mempool_init(struct MEMPOOL* pool, size_t bucket_size);

// Release a memory pool and all its allocations
void mempool_release(struct MEMPOOL* pool);

// Allocate memory from the pool (never released until entire pool released)
void* mempool_alloc(struct MEMPOOL* pool, size_t cb);

// Allocate a copy of existing memory buffer
void* mempool_alloc_copy(struct MEMPOOL* pool, const void* p, size_t cb);

// Allocate a copy of a null terminated string
char* mempool_alloc_str(struct MEMPOOL* pool, const char* psz);