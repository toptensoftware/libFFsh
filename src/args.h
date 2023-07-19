#pragma once

#include "ffsh.h"

// Indicies into special_arg_chars
#define SPECIAL_CHAR_STAR       0
#define SPECIAL_CHAR_QUESTION   1
#define SPECIAL_CHAR_OPENBRACE  2
#define SPECIAL_CHAR_CLOSEBRACE 3
#define SPECIAL_CHAR_COMMA      4

extern uint32_t special_arg_chars[];

// --- Basic argc parsing and helpers ---

// Count number of args
int count_argv(char* psz);

// Parse a command line string
void parse_argv(char* psz, ARGS* pargs, int maxargc);

// Remove a command line arg
void remove_arg(ARGS* pargs, int position);

// Split arguments
bool split_args(ARGS* pargs, int position, ARGS* pTailArgs);

// Restore a special character to it's original non-special version
uint32_t restore_brace_special_char(uint32_t cp);
void restore_brace_special_chars(char* psz);
uint32_t restore_glob_special_char(uint32_t cp);
void restore_glob_special_chars(char* psz);
