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


bool parse_millis(const char* p, uint32_t* pValue)
{
    // Skip whitespace
    while (*p == ' ' || *p == '\t')
        p++;

    // Must have a digit
    if ((*p < '0' || *p > '9') && *p != '.')
        return false;

    // Parse the whole number part
    uint32_t whole = 0;
    while (*p >= '0' && *p <= '9')
    {
        whole = whole * 10 + (*p - '0');
        p++;
    }

    // Parse the fractional part (if present)
    uint32_t fraction = 0;
    if (*p == '.')
    {
        p++;
        uint32_t column = 100000;
        while (*p >= '0' && *p <= '9')
        {
            fraction = fraction + column * (*p - '0');
            column /= 10;
            p++;
        }
    }

    // Handle suffix
    switch (*p)
    {
        case 's':
            p++;
            *pValue = whole * 1000 + (fraction / 1000);
            break;

        case 'm':
            if (p[1] == 's')
            {
                // Milliseconds
                p+=2;
                if (fraction >= 500000)
                    *pValue = whole + 1;
                else
                    *pValue = whole; 
            }
            else
            {
                // Minutes
                p++;
                *pValue = whole * 1000 * 60 + (fraction * 60 / 1000);
            }
            break;

        case 'h':
            // Hours
            p++;
            *pValue = whole * 1000 * 60 * 60  + (fraction * 60 * 60 / 1000);
            break;

        case 'd':
            // Days
            p++;
            *pValue = whole * 1000 * 60 * 60 * 24 + (fraction * 60 * 60 * 24 / 1000);
            break;

        default:
            // Seconds
            *pValue = whole * 1000 + (fraction / 1000);
            break;

    }

    while (*p == ' ' || *p == '\t')
        p++;

    if (*p)
        return false;

    return true;
}
