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

    int state;
    int index;
    int err;

    // Opt enumeration
    char saveDelim;
    char* delimPos;
    const char* pszShort;

    // Brace expansion
    int bracePerms;
    int bracePerm;
    char braceOps[128];

    // Wildcard matching
    bool didMatch;
    DIR dir;
    FILINFO fi;
    const char* pszBase;
    const char* pszRelBase;

    // Buffers
    char szArg[FF_MAX_LFN];
    char sz[FF_MAX_LFN];
    char szAbs[FF_MAX_LFN];
    char szRel[FF_MAX_LFN];
} ENUM_ARGS;

typedef struct {
    const char* pszOpt;
    const char* pszValue;
} OPT;

typedef struct
{
    const char* pszRelative;
    const char* pszAbsolute;
    FILINFO* pfi;
} ARG;

// Begins expansion of path arguments
void start_enum_args(ENUM_ARGS* pctx, CMD_CONTEXT* pcmd, ARGS* pargs);

// Get the next option
bool next_opt(ENUM_ARGS* pctx, OPT* popt);

// Check if option matches pszOptNames (eg: "-n|--longname")
bool is_opt(const char* pszOpt, const char* pszOptNames);

// Check if popt matches opt name, and no value
bool is_switch(ENUM_ARGS* pctx, OPT* popt, const char* pszOptNames);

// Check if popt matches opt name, with value
bool is_value_opt(ENUM_ARGS* pctx, OPT* popt, const char* pszOptNames);

// Unknown opt handler
void unknown_opt(ENUM_ARGS* pctx, OPT* popt);

// Get the next argument
bool next_arg(ENUM_ARGS* pctx, ARG* ppath);

// Finish expansion of path args
int end_enum_args(ENUM_ARGS* pctx);

// Check if error
int enum_args_error(ENUM_ARGS* pctx);

// Abort argument enumeration
void abort_enum_args(ENUM_ARGS* pctx, int err);

// Set error code to be returned from end_enum_args, but continue enumeration
void set_enum_args_error(ENUM_ARGS* pctx, int err);



