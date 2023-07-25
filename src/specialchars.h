#pragma once

#include <stdint.h>

// Indicies into special_chars
#define SPECIAL_CHAR_STAR       0
#define SPECIAL_CHAR_QUESTION   1
#define SPECIAL_CHAR_OPENBRACE  2
#define SPECIAL_CHAR_CLOSEBRACE 3
#define SPECIAL_CHAR_COMMA      4

extern uint32_t special_chars[];

uint32_t map_special_char(uint32_t codepoint);

// Restore a special character to it's original non-special version
// '{' ',' '}'
uint32_t restore_brace_special_char(uint32_t cp);
void restore_brace_special_chars(char* psz);

// '*' '?'
uint32_t restore_glob_special_char(uint32_t cp);
void restore_glob_special_chars(char* psz);

