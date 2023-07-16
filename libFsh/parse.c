#include "common.h"
#include "parse.h"

int parseNibble(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 0xA;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 0xA;
    return -1;
}

int parseDigit(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    return -1;
}

bool parse_uint32(const char* p, uint32_t* pValue)
{
    *pValue = 0;

    // Skip leading whitespace
    while (*p == ' ' || *p == '\t')
        p++;

    // Hex or decimal
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
    {
        p+=2;
        while (true)
        {
            int nib = parseNibble(*p);
            if (nib < 0)
                break;
            *pValue = *pValue << 4 | nib;
            p++;
        }
    }
    else
    {
        while (true)
        {
            int digit = parseNibble(*p);
            if (digit < 0)
                break;
            *pValue = *pValue * 10 + digit;
            p++;
        }
    }

    // Skip trailing whitespace
    while (*p == ' ' || *p == '\t')
        p++;

    // Check end of string
    if (*p)
        return false;

    return true;
}

