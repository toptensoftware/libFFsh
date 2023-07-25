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

static int (*g_compare)(void* user, const DIRENTRY* a, const DIRENTRY* b);
static void* g_user;

int direntry_compare_name(void* user, const DIRENTRY* a, const DIRENTRY* b)
{
    return utf8cmpi(a->fname, b->fname);
}

bool direntry_filter_hidden(void* user, FILINFO* pfi)
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

    return g_compare(g_user, a, b);
}

int f_opendir_ex(DIREX* direx, const char* pszDir, void* user, 
    bool (*filter)(void* user, FILINFO* pfi),
    int (*compare)(void* user, const DIRENTRY* a, const DIRENTRY* b)
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
        if (filter != NULL && !filter(user, &fi))
            continue;

        // Allocate entry
        DIRENTRY* pde = direntry_alloc(&direx->pool, &fi);

        // Add to list
        memstream_write(&direx->stream, &pde, sizeof(pde));
    }

    if (compare != NULL)
    {
        g_compare = compare;
        g_user = user;
        qsort(direx->stream.p, direx->stream.length / sizeof(void*), sizeof(void*), direntry_compare_stub);
        g_compare = NULL;
        g_user = NULL;
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