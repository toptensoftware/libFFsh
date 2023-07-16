#pragma once

#include <stdint.h>
#include <stddef.h>

uint32_t utf8_decode(const char** ptr);
size_t utf8_encode(uint32_t codepoint, char* p, size_t room);
uint32_t utf32_toupper(uint32_t codepoint);
