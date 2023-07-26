#include <malloc.h>
#include <string.h>
#include "mempool.h"

// Initialize a new memory pool
void mempool_init(struct MEMPOOL* pool, size_t bucket_size)
{
    pool->bucket_size = bucket_size;
    pool->first = NULL;
}

// Release a memory pool and all its allocations
void mempool_release(struct MEMPOOL* pool)
{
    struct MEMPOOL_BUCKET* p = pool->first;
    while (p)
    {
        struct MEMPOOL_BUCKET* next = p->next;
        free(p);
        p = next;
    }
    pool->first = NULL;
}

// Allocate memory from the pool
void* mempool_alloc(struct MEMPOOL* pool, size_t cb)
{
    // Zero sized alloc?
    if (cb == 0)
        return NULL;

    // Keep allocations aligned to pointer size
    if (cb % sizeof(void*))
        cb += sizeof(void*) - cb % sizeof(void*);

    // Try all existing buckets
    struct MEMPOOL_BUCKET* p = pool->first;
    while (p)
    {
        if (p->used + cb < p->size)
        {
            void* mem = (char*)p + p->used;
            p->used += cb;
            return mem;
        }
        p = p->next;
    }

    // Create a new bucket
    size_t bucket_size = cb < pool->bucket_size / 2 ? pool->bucket_size : cb + sizeof(struct MEMPOOL_BUCKET);
    p = (struct MEMPOOL_BUCKET*)malloc(bucket_size);
    if (p == NULL)
        return NULL;

    // Initialize bucket
    p->size = bucket_size;
    p->used = sizeof(struct MEMPOOL_BUCKET) + cb;

    // Add to linked list
    p->next = pool->first;
    pool->first = p;

    // Return memory ptr
    return (char*)p->memory;
}

// Allocate a copy of existing memory buffer
void* mempool_alloc_copy(struct MEMPOOL* pool, const void* p, size_t cb)
{
    // Alloc
    void* pCopy = mempool_alloc(pool, cb);
    if (pCopy == NULL)
        return pCopy;

    // Copy
    memcpy(pCopy, p, cb);
    return pCopy;
}

// Allocate a copy of a null terminated string
char* mempool_alloc_str(struct MEMPOOL* pool, const char* psz)
{
    if (psz == NULL)
        return NULL;

    return (char*)mempool_alloc_copy(pool, psz, strlen(psz) + 1);
}