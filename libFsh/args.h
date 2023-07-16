#pragma once

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

// Split arguments
bool split_args(ARGS* pargs, int position, ARGS* pTailArgs);
