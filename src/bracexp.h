#pragma once

#include <stddef.h>

// Prepare a braced expression for expansion
int bracexp_prepare(const char* psz, uint32_t* special_chars, void* pOps, size_t cbBuf);

// Expand a previously prepared braced expression
void bracexp_expand(char* pDest, int perm, const void* pOps);