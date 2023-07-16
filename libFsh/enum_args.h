#pragma once

#include <stdbool.h>
#include <ff.h>

#include "args.h"
#include "commands.h"

// --- Argument Expansion ---

// Expand a sequence of paths
typedef struct
{
    CMD_CONTEXT* pcmd;
    ARGS* pargs;
    int index;
    bool inFind;
    int err;
    DIR dir;
    FILINFO fi;
    const char* pszBase;
    const char* pszRelBase;
    const char* pszArg;
    char sz[FF_MAX_LFN];
    char szAbs[FF_MAX_LFN];
    char szRel[FF_MAX_LFN];
} ENUM_ARGS;
typedef struct
{
    const char* pszRelative;
    const char* pszAbsolute;
    FILINFO* pfi;
} ARG;

// Begins expansion of path arguments
void start_enum_args(ENUM_ARGS* pctx, CMD_CONTEXT* pcmd, ARGS* pargs);

// Get the next argument
bool next_arg(ENUM_ARGS* pctx, ARG* ppath);

// Finish expansion of path args
int end_enum_args(ENUM_ARGS* pctx);

// Abort argument enumeration
void abort_enum_args(ENUM_ARGS* pctx, int err);

// Set error code to be returned from end_enum_args, but continue enumeration
void set_enum_args_error(ENUM_ARGS* pctx, int err);



