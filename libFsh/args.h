#pragma once

// Indicies into special_arg_chars
#define SPECIAL_CHAR_STAR       0
#define SPECIAL_CHAR_QUESTION   1
#define SPECIAL_CHAR_OPENBRACE  2
#define SPECIAL_CHAR_CLOSEBRACE 3
#define SPECIAL_CHAR_COMMA      4

extern uint32_t special_arg_chars[];

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

// Restore special characters to their default
void restore_special_args(char* psz);
