#pragma once

#include <stdbool.h>
#include <ff.h>

FRESULT f_stat_ex(
    const TCHAR* path,	/* Pointer to the file path */
	FILINFO* fno		/* Pointer to file information to return */
);

// Map a FatFS error code to standard error code
int f_maperr(int err);

// Get a descriptive error message for a FatFS error code
const char* f_strerror(int err);

// Check if file is hidden
bool f_is_hidden(FILINFO* pfi);

// Copy a file
int f_copyfile(const char* pszDest, const char* pszSrc, bool optOverwrite, void (*progress)());

// Recursively remove a directory
// psz buffer should be at least FF_MAX_LFN in size
int f_rmdir_r(char* psz, void (*progress)());

// Recursively create a directory
int f_mkdir_r(const char* psz);
