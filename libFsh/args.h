#pragma once

// These tokens are used to replace unquoted, unescaped
// characters in the input stream
#define TOKEN_STAR          '\x01'          // *
#define TOKEN_QUESTION      '\x02'          // ?
#define TOKEN_OPENBRACE     '\x03'          // {
#define TOKEN_CLOSEBRACE    '\x04'          // }
#define TOKEN_COMMA         '\x05'          // ,


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
