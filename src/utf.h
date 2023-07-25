#pragma once

#include <stdint.h>
#include <stddef.h>

uint32_t utf8_decode(const char** ptr);
void utf8_encode(uint32_t codepoint, char** p);
size_t utf8_encode_ex(uint32_t codepoint, char* p, size_t room);
uint32_t utf32_toupper(uint32_t codepoint);

struct UTF8
{
    const char* pcurr;
    const char* pnext;
    uint32_t codepoint;
};

uint32_t utf8_init(struct UTF8* pctx, const char* psz);
uint32_t utf8_next(struct UTF8* pctx);

int utf8cmpi(const char* a, const char* b);