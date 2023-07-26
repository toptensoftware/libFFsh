#pragma once

#include <stdbool.h>

#include <ff.h>
#include "mempool.h"
#include "memstream.h"

// Same as FatFS but allocated in pool
typedef struct 
{
    const char* fname;
	FSIZE_t	fsize;			/* File size */
	WORD	fdate;			/* Modified date */
	WORD	ftime;			/* Modified time */
	BYTE	fattrib;		/* File attribute */
} DIRENTRY;

DIRENTRY* direntry_alloc(struct MEMPOOL* pool, FILINFO* pfi);

int direntry_compare_name(void* user, const DIRENTRY* a, const DIRENTRY* b);
int direntry_compare_size(void* user, const DIRENTRY* a, const DIRENTRY* b);
int direntry_compare_time(void* user, const DIRENTRY* a, const DIRENTRY* b);

bool direntry_filter_hidden(void* user, FILINFO* pfi);

typedef struct
{
	struct MEMPOOL pool;
	struct MEMSTREAM stream;
	int index;
} DIREX;


int f_opendir_ex(DIREX* direx, const char* pszDir, void* user, 
    bool (*filter)(void* user, FILINFO* pfi),
    int (*compare)(void* user, const DIRENTRY* a, const DIRENTRY* b)
    );
bool f_readdir_ex(DIREX* dir, DIRENTRY** ppde);
int f_closedir_ex(DIREX* dir);