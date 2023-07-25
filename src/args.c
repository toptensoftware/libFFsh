#include "common.h"
#include "utf.h"
#include "args.h"

/*

bool isWhitespace(uint32_t codepoint)
{
    return codepoint == ' ' || 
            codepoint == '\t' || 
            codepoint == '\r' || 
            codepoint == '\n';
}


int count_argv(char* psz)
{
    struct UTF8 u;
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


        if (u.codepoint == '&')
        {
            utf8_next(&u);
            if (u.codepoint == '&')
            {
                utf8_next(&u);
            }
            count++;
            continue;
        }

        if (u.codepoint == '|')
        {
            utf8_next(&u);
            if (u.codepoint == '|')
            {
                utf8_next(&u);
            }
            count++;
            continue;
        }

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
                break;
            }

            // Special character
            if (u.codepoint == '&' || u.codepoint == '|')
            {
                break;
            }


            utf8_next(&u);
        }
    }
    return count;
}


// Parse a command line
// Re-uses the psz buffer and overwrites it with the "parsed" strings.
void parse_argv(char* psz, struct ARGS* pargs, int maxargc)
{
    pargs->argc = 0;
    char* pszDest = psz;

    struct UTF8 u;
    utf8_init(&u, psz);

    while (u.codepoint && pargs->argc < maxargc)
    {
        // Skip white space
        while (isWhitespace(u.codepoint))
            utf8_next(&u);

        // End of string
        if (!u.codepoint)
            break;

        if (u.codepoint == '&')
        {
            utf8_next(&u);
            if (u.codepoint == '&')
            {
                utf8_next(&u);
                pargs->argv[pargs->argc] = token_logical_and;
                pargs->argc++;
            }
            else
            {
                pargs->argv[pargs->argc] = token_bg;
                pargs->argc++;
            }
            continue;
        }

        if (u.codepoint == '|')
        {
            utf8_next(&u);
            if (u.codepoint == '|')
            {
                utf8_next(&u);
                pargs->argv[pargs->argc] = token_logical_or;
                pargs->argc++;
            }
            else
            {
                pargs->argv[pargs->argc] = token_pipe;
                pargs->argc++;
            }
            continue;
        }

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
                    utf8_encode(u.codepoint, &pszDest);
                }
                continue;
            }

            // Quoted?
            if (u.codepoint == '\"')
            {
                utf8_next(&u);
                while (u.codepoint && u.codepoint != '\"')
                {
                    utf8_encode(u.codepoint, &pszDest);
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
                    utf8_encode(u.codepoint, &pszDest);
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

            // Special character
            if (u.codepoint == '&' || u.codepoint == '|')
            {
                *pszDest++ = '\0';
                break;
            }

            // Convert special characters to tokens
            switch (u.codepoint)
            {
                case '*': u.codepoint = special_chars[SPECIAL_CHAR_STAR]; break;
                case '?': u.codepoint = special_chars[SPECIAL_CHAR_QUESTION]; break;
                case '{': u.codepoint = special_chars[SPECIAL_CHAR_OPENBRACE]; break;
                case '}': u.codepoint = special_chars[SPECIAL_CHAR_CLOSEBRACE]; break;
                case ',': u.codepoint = special_chars[SPECIAL_CHAR_COMMA]; break;
            }

            // Other char
            utf8_encode(u.codepoint, &pszDest);
            utf8_next(&u);
        }
    }

    *pszDest = '\0';
}
*/

void remove_arg(struct ARGS* pargs, int position)
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

bool split_args(struct ARGS* pargs, int position, struct ARGS* pTailArgs)
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


