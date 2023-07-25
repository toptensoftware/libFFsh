#pragma once

#include "utf.h"
#include "memstream.h"
#include "mempool.h"
#include "specialchars.h"

// Tokens
#define TOKEN_EOF           0       // End of input string
#define TOKEN_ARG           1       // command line argument, with special chars
#define TOKEN_PIPE          2       // '|'
#define TOKEN_BG            3       // '&'
#define TOKEN_LOGICAL_OR    4       // '||'
#define TOKEN_LOGICAL_AND   5       // '&&'
#define TOKEN_SEMICOLON     6       // ';'
#define TOKEN_OPENROUND     7       // '('
#define TOKEN_CLOSEROUND    8       // ')'

struct TOKENIZER
{
// Current token
    int token;                      // Token type
    const char* arg;                // Text of the current arg

// Internal
    const char*      psz;           // The original string
    struct MEMPOOL*  pool;          // Memory pool
    struct UTF8      utf8;          // utf8 decoder  
    struct MEMSTREAM builder;       // String builder
};

void tokenizer_init(struct TOKENIZER* tokenizer, struct MEMPOOL* pool, const char* psz);
void tokenizer_next(struct TOKENIZER* tokenizer);
void tokenizer_close(struct TOKENIZER* tokenizer);
