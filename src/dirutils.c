#include <stdlib.h>

#include "dirutils.h"
#include "utf.h"
#include "ffex.h"

DIRENTRY* direntry_alloc(struct MEMPOOL* pool, FILINFO* pfi)
{
    DIRENTRY* pde = (DIRENTRY*)mempool_alloc(pool, sizeof(DIRENTRY));
    pde->fname = mempool_alloc_str(pool, pfi->fname);
    pde->fsize = pfi->fsize;
    pde->fdate = pfi->fdate;
    pde->ftime = pfi->ftime;
    pde->fattrib = pfi->fattrib;
    return pde;
}

static int (*g_compare)(void* ctx, const DIRENTRY* a, const DIRENTRY* b);
static void* g_compare_ctx;

int direntry_compare_name(void* ctx, const DIRENTRY* a, const DIRENTRY* b)
{
    int order = ctx == NULL ? 1 : *(int*)ctx;
    return utf8cmpi(a->fname, b->fname) * order;
}

int direntry_compare_size(void* ctx, const DIRENTRY* a, const DIRENTRY* b)
{
    int order = ctx == NULL ? 1 : *(int*)ctx;
    if (a->fsize > b->fsize)
        return 1 * order;
    if (a->fsize < b->fsize)
        return -1 * order;
    return direntry_compare_name(ctx, a, b);
}

int direntry_compare_time(void* ctx, const DIRENTRY* a, const DIRENTRY* b)
{
    int order = ctx == NULL ? 1 : *(int*)ctx;
    if (a->fdate > b->fdate)
        return 1 * order;
    if (a->fdate < b->fdate)
        return -1 * order;
    if (a->ftime > b->ftime)
        return 1 * order;
    if (a->ftime < b->ftime)
        return -1 * order;
    return direntry_compare_name(ctx, a, b);
}


bool direntry_filter_hidden(void* ctx, FILINFO* pfi)
{
    if (f_is_hidden(pfi))
        return false;
    return true;
}

int direntry_compare_stub(const void* aptr, const void* bptr)
{
    // Get the pointers
    DIRENTRY* a = *(DIRENTRY**)aptr;
    DIRENTRY* b = *(DIRENTRY**)bptr;

    return g_compare(g_compare_ctx, a, b);
}

int f_opendir_ex(DIREX* direx, const char* pszDir, 
    void* filter_ctx, bool (*filter)(void* ctx, FILINFO* pfi),
    void* compare_ctx, int (*compare)(void* ctx, const DIRENTRY* a, const DIRENTRY* b)
    )
{
    // Open directory
    DIR dir;
    int err = f_opendir(&dir, pszDir);
    if (err)
        return err;

    // Initialize mem storage
    mempool_init(&direx->pool, 4096);
    memstream_initnew(&direx->stream, 4096);
    direx->index = 0;

    // Read all entries
    while (true)
    {
        // Read next entry
        FILINFO fi;
        err = f_readdir(&dir, &fi);
        if (err)
        {
            f_closedir_ex(direx);
            f_closedir(&dir);
            return err;
        }
        if (fi.fname[0] == 0)
            break;

        // Filter unwanted
        if (filter != NULL && !filter(filter_ctx, &fi))
            continue;

        // Allocate entry
        DIRENTRY* pde = direntry_alloc(&direx->pool, &fi);

        // Add to list
        memstream_write(&direx->stream, &pde, sizeof(pde));
    }

    if (compare != NULL)
    {
        g_compare = compare;
        g_compare_ctx = compare_ctx;
        qsort(direx->stream.p, direx->stream.length / sizeof(void*), sizeof(void*), direntry_compare_stub);
        g_compare = NULL;
        g_compare_ctx = NULL;
    }

    f_closedir(&dir);
    return 0;
}

bool f_readdir_ex(DIREX* direx, DIRENTRY** ppde)
{
    // Check for end
    int count = memstream_length(&direx->stream) / sizeof(void*);
    if (direx->index >= count)
    {
        *ppde = NULL;
        return false;
    }

    // Return entry
    *ppde = ((DIRENTRY**)(direx->stream.p))[direx->index++];
    return true;
}

int f_closedir_ex(DIREX* direx)
{
    mempool_release(&direx->pool);
    memstream_close(&direx->stream);
    return 0;
}