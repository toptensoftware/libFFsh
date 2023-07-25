#include <stdbool.h>

#include "specialchars.h"
#include "utf.h"


// From unicode private use area
uint32_t special_chars[] = {
    0xF801, 0xF802, 0xF803, 0xF804, 0xF805, 
};


uint32_t map_special_char(uint32_t codepoint)
{
    switch (codepoint)
    {
        case '*': return special_chars[SPECIAL_CHAR_STAR]; break;
        case '?': return special_chars[SPECIAL_CHAR_QUESTION]; break;
        case '{': return special_chars[SPECIAL_CHAR_OPENBRACE]; break;
        case '}': return special_chars[SPECIAL_CHAR_CLOSEBRACE]; break;
        case ',': return special_chars[SPECIAL_CHAR_COMMA]; break;
    }
    return codepoint;
}


uint32_t restore_brace_special_char(uint32_t cp)
{
    if (cp == special_chars[SPECIAL_CHAR_OPENBRACE])
        return '{';
    else if (cp == special_chars[SPECIAL_CHAR_CLOSEBRACE])
        return '}';
    else if (cp == special_chars[SPECIAL_CHAR_COMMA])
        return ',';
    return cp;
}


void restore_brace_special_chars(char* psz)
{
    char* pDest = psz;
    while (true)
    {
        uint32_t cp = utf8_decode((const char**)&psz);

        cp = restore_brace_special_char(cp);

        utf8_encode(cp, &pDest);
        if (cp == 0)
            break;
    }
}


uint32_t restore_glob_special_char(uint32_t cp)
{
    if (cp == special_chars[SPECIAL_CHAR_STAR])
        return '*';
    else if (cp == special_chars[SPECIAL_CHAR_QUESTION])
        return '?';
    return cp;
}


void restore_glob_special_chars(char* psz)
{
    char* pDest = psz;
    while (true)
    {
        uint32_t cp = utf8_decode((const char**)&psz);

        cp = restore_glob_special_char(cp);

        utf8_encode(cp, &pDest);
        if (cp == 0)
            break;
    }
}

