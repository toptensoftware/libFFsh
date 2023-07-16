#pragma once

#include <stdint.h>
#include <stddef.h>

uint32_t utf8_decode(const char** ptr);
size_t utf8_encode(uint32_t codepoint, char* p, size_t room);
uint32_t utf32_toupper(uint32_t codepoint);

typedef struct
{
    const char* pcurr;
    const char* pnext;
    uint32_t codepoint;
} UTF8;

uint32_t utf8_init(UTF8* pctx, const char* psz);
uint32_t utf8_next(UTF8* pctx);