#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "args.h"

// --- Option Enumeration ---

// Option enumeration
typedef struct {
    ARGS* pargs;
    int index;
    char saveDelim;
    char* delimPos;
    char szTemp[6];         // '-' + up to 4 utf8 chars + '\0'
    const char* pszShort;
} ENUM_OPTS;

typedef struct {
    const char* pszOpt;
    const char* pszValue;
} OPT;

// Begin enumerate opts
void start_enum_opts(ENUM_OPTS* pctx, ARGS* pargs);

// Get the next opt
bool next_opt(ENUM_OPTS* pctx, OPT* popt);

