#include "tokenizer.h"

static bool is_whitespace(uint32_t codepoint)
{
    return codepoint == ' ' || 
            codepoint == '\t' || 
            codepoint == '\r' || 
            codepoint == '\n';
}

// Encode a single codepoint to the tokenizer's builder
static void encode(struct TOKENIZER* tokenizer, uint32_t codepoint)
{
    // Reserve space
    void* dest = memstream_reserve(&tokenizer->builder, 4);

    // Encode it and skip over it
    memstream_alloc(&tokenizer->builder, utf8_encode_ex(codepoint, dest, 4));
}


static void parse_arg(struct TOKENIZER* tokenizer)
{
    while (tokenizer->utf8.codepoint)
    {
        // End of arg?
        if (is_whitespace(tokenizer->utf8.codepoint))
        {
            return;
        }

        // Backslash escaped?
        switch (tokenizer->utf8.codepoint)
        {
            case '\\':
                utf8_next(&tokenizer->utf8);
                if (tokenizer->utf8.codepoint)
                {
                    encode(tokenizer, tokenizer->utf8.codepoint);
                    utf8_next(&tokenizer->utf8);
                }
                break;

            case '\"':
                utf8_next(&tokenizer->utf8);
                while (tokenizer->utf8.codepoint && tokenizer->utf8.codepoint != '\"')
                {
                    encode(tokenizer, tokenizer->utf8.codepoint);
                    utf8_next(&tokenizer->utf8);
                }
                utf8_next(&tokenizer->utf8);
                break;

            case '\'':
                utf8_next(&tokenizer->utf8);
                while (tokenizer->utf8.codepoint && tokenizer->utf8.codepoint != '\'')
                {
                    encode(tokenizer, tokenizer->utf8.codepoint);
                    utf8_next(&tokenizer->utf8);
                }
                utf8_next(&tokenizer->utf8);
                break;


            case '&':
            case '|':
            case ';':
            case '(':
            case ')':
                return;

            default:
                if (tokenizer->escape_special_chars)
                    encode(tokenizer, map_special_char(tokenizer->utf8.codepoint));
                else
                    encode(tokenizer, tokenizer->utf8.codepoint);
                utf8_next(&tokenizer->utf8);
                break;
        }
    }
}


void tokenizer_init(struct TOKENIZER* tokenizer, struct MEMPOOL* pool,  bool specialchars, const char* psz)
{
    // Store params
    tokenizer->pool = pool;
    tokenizer->psz = psz;
    tokenizer->escape_special_chars = specialchars;

    // Init UTF8 decoder
    utf8_init(&tokenizer->utf8, psz);

    // Init string builder
    memstream_initnew(&tokenizer->builder, 512);

    // Load first token
    tokenizer_next(tokenizer);
}

void tokenizer_next(struct TOKENIZER* tokenizer)
{
    // Skip whitespace
    while (is_whitespace(tokenizer->utf8.codepoint))
        utf8_next(&tokenizer->utf8);

    // Handle token
    switch (tokenizer->utf8.codepoint)
    {
        // End of string
        case '\0':
            tokenizer->token = TOKEN_EOF;
            return;

        case '&':
            utf8_next(&tokenizer->utf8);
            if (tokenizer->utf8.codepoint == '&')
            {
                utf8_next(&tokenizer->utf8);
                tokenizer->token = TOKEN_LOGICAL_AND;
            }
            else
            {
                tokenizer->token = TOKEN_BG;
            }
            return;

        case '|':
            utf8_next(&tokenizer->utf8);
            if (tokenizer->utf8.codepoint == '|')
            {
                utf8_next(&tokenizer->utf8);
                tokenizer->token = TOKEN_LOGICAL_OR;
            }
            else
            {
                tokenizer->token = TOKEN_PIPE;
            }
            return;

        case ';':
            utf8_next(&tokenizer->utf8);
            tokenizer->token = TOKEN_SEMICOLON;
            return;

        case '(':
            utf8_next(&tokenizer->utf8);
            tokenizer->token = TOKEN_OPENROUND;
            return;

        case ')':
            utf8_next(&tokenizer->utf8);
            tokenizer->token = TOKEN_CLOSEROUND;
            return;

        default:
            // Reset string builder
            memstream_reset(&tokenizer->builder);
            
            // Parse the arg
            parse_arg(tokenizer);

            // Encode terminator
            encode(tokenizer, 0);

            // Allocate a copy
            tokenizer->arg = (const char*)mempool_alloc_copy(
                tokenizer->pool, tokenizer->builder.p, tokenizer->builder.length);
            tokenizer->token = TOKEN_ARG;
            return;
    }
}


void tokenizer_close(struct TOKENIZER* tokenizer)
{
    memstream_close(&tokenizer->builder);
}
