#include "common.h"
#include "utf.h"
#include "args.h"

uint32_t special_arg_chars[] = {
    1, 2, 3, 4, 5
};


bool isWhitespace(uint32_t codepoint)
{
    return codepoint == ' ' || 
            codepoint == '\t' || 
            codepoint == '\r' || 
            codepoint == '\n';
}

int count_argv(char* psz)
{
    UTF8 u;
    utf8_init(&u, psz);

    int count = 0;
    while (u.codepoint)
    {
        // Skip white space
        while (isWhitespace(u.codepoint))
            utf8_next(&u);

        // End of string
        if (!u.codepoint)
            break;

        // Bump count
        count++;

        // Skip arg
        while (u.codepoint)
        {
            // Backslash escaped?
            if (u.codepoint == '\\')
            {
                utf8_next(&u);
                utf8_next(&u);
                continue;
            }

            // Quoted?
            if (u.codepoint == '\"')
            {
                utf8_next(&u);
                while (u.codepoint && u.codepoint != '\"')
                {
                    utf8_next(&u);
                    psz++;
                }
                utf8_next(&u);
                continue;
            }

            if (u.codepoint == '\'')
            {
                utf8_next(&u);
                while (u.codepoint && u.codepoint != '\'')
                {
                    utf8_next(&u);
                    psz++;
                }
                utf8_next(&u);
                continue;
            }

            // End of arg?
            if (isWhitespace(u.codepoint))
            {
                utf8_next(&u);
                break;
            }

            utf8_next(&u);
        }
    }
    return count;
}

// Parse a command line
// Re-uses the psz buffer and overwrites it with the "parsed" strings.
void parse_argv(char* psz, ARGS* pargs, int maxargc)
{
    pargs->argc = 0;
    char* pszDest = psz;

    UTF8 u;
    utf8_init(&u, psz);

    while (u.codepoint && pargs->argc < maxargc)
    {
        // Skip white space
        while (isWhitespace(u.codepoint))
            utf8_next(&u);

        // End of string
        if (!u.codepoint)
            break;

        // Store start of string
        pargs->argv[pargs->argc] = pszDest;
        pargs->argc++;
        while (u.codepoint)
        {
            // Backslash escaped?
            if (u.codepoint == '\\')
            {
                utf8_next(&u);
                if (u.codepoint)
                {
                    pszDest += utf8_encode(u.codepoint, pszDest, 4);
                }
                continue;
            }

            // Quoted?
            if (u.codepoint == '\"')
            {
                utf8_next(&u);
                while (u.codepoint && u.codepoint != '\"')
                {
                    pszDest += utf8_encode(u.codepoint, pszDest, 4);
                    utf8_next(&u);
                }
                utf8_next(&u);
                continue;
            }

            if (u.codepoint == '\'')
            {
                utf8_next(&u);
                while (u.codepoint && u.codepoint != '\'')
                {
                    pszDest += utf8_encode(u.codepoint, pszDest, 4);
                    utf8_next(&u);
                }
                utf8_next(&u);
                continue;
            }

            // End of arg?
            if (isWhitespace(u.codepoint))
            {
                *pszDest++ = '\0';
                break;
            }

            // Convert special characters to tokens
            switch (u.codepoint)
            {
                case '*': u.codepoint = special_arg_chars[SPECIAL_CHAR_STAR]; break;
                case '?': u.codepoint = special_arg_chars[SPECIAL_CHAR_QUESTION]; break;
                case '{': u.codepoint = special_arg_chars[SPECIAL_CHAR_OPENBRACE]; break;
                case '}': u.codepoint = special_arg_chars[SPECIAL_CHAR_CLOSEBRACE]; break;
                case ',': u.codepoint = special_arg_chars[SPECIAL_CHAR_COMMA]; break;
            }

            // Other char
            pszDest += utf8_encode(u.codepoint, pszDest, 4);
            utf8_next(&u);
        }
    }

    *pszDest = '\0';
}

void remove_arg(ARGS* pargs, int position)
{
    // Check valid
    if (position < 0 || position >= pargs->argc)
        return;

    // Shift following items back
    int nShift = pargs->argc - position - 1;
    memcpy(pargs->argv + position, pargs->argv + position + 1, nShift * sizeof(const char*));

    // Update count
    pargs->argc--;
}

bool split_args(ARGS* pargs, int position, ARGS* pTailArgs)
{
    // Split from end
    if (position < 0)
        position = pargs->argc + position;

    // Past end?
    if (position >= pargs->argc)
    {
        pTailArgs->argc = 0;
        pTailArgs->argv = NULL;
        return false;
    }

    // Split
    pTailArgs->argv = pargs->argv + position;
    pTailArgs->argc = pargs->argc - position;
    pargs->argc = position;
    return true;
}


uint32_t restore_brace_special_char(uint32_t cp)
{
    if (cp == special_arg_chars[SPECIAL_CHAR_OPENBRACE])
        return '{';
    else if (cp == special_arg_chars[SPECIAL_CHAR_CLOSEBRACE])
        return '}';
    else if (cp == special_arg_chars[SPECIAL_CHAR_COMMA])
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

        pDest += utf8_encode(cp, pDest, 4);
        if (cp == 0)
            break;
    }
}


uint32_t restore_glob_special_char(uint32_t cp)
{
    if (cp == special_arg_chars[SPECIAL_CHAR_STAR])
        return '*';
    else if (cp == special_arg_chars[SPECIAL_CHAR_QUESTION])
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

        pDest += utf8_encode(cp, pDest, 4);
        if (cp == 0)
            break;
    }
}
