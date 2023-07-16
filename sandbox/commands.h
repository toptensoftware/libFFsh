#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// --- Basic argc parsing and helpers ---

// Helper to store argv/argc
typedef struct 
{
    const char** argv;
    int argc;
} ARGS;

// Count number of args
int count_argv(char* psz);

// Parse a command line string
void parse_argv(char* psz, ARGS* pargs, int maxargc);

// Remove a command line arg
void remove_arg(ARGS* pargs, int position);


// --- Option Enumeration ---

// Option enumeration
typedef struct {
    ARGS* pargs;
    int index;
    char saveDelim;
    char* delimPos;
    char szTemp[3];
    const char* pszShort;
} ENUM_OPTS;

typedef struct {
    const char* pszOpt;
    const char* pszValue;
} OPT;

// Begin enumerate opts
void enum_opts(ENUM_OPTS* pctx, ARGS* pargs);

// Get the next opt
bool next_opt(ENUM_OPTS* pctx, OPT* popt);


// --- Argument Expansion ---

// Expand a sequence of paths
typedef struct
{
    const char* cwd;
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
void start_enum_args(ENUM_ARGS* pctx, const char* cwd, ARGS* pargs);

// Get the next argument
bool next_arg(ENUM_ARGS* pctx, ARG* ppath);

// Finish expansion of path args
int end_enum_args(ENUM_ARGS* pctx);

// Abort argument enumeration
void abort_enum_args(ENUM_ARGS* pctx, int err);

// Set error code to be returned from end_enum_args, but continue enumeration
void set_enum_args_error(ENUM_ARGS* pctx, int err);



// --- Command context ---

// Command context
typedef struct
{
    ARGS* pargs;
    char* cwd;
} CMD_CONTEXT;

// Commands
int cmd_touch(CMD_CONTEXT* pctx);

int cmd(CMD_CONTEXT* pctx);