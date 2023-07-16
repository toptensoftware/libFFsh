#include <stdbool.h>
#include <string.h>
#include "args.h"


int count_argv(char* psz)
{
    int count = 0;
    while (*psz)
    {
        // Skip white space
        while (*psz == ' ' || *psz == '\t')
            psz++;

        // End of string
        if (!*psz)
            break;

        // Bump count
        count++;

        // Skip arg
        while (*psz)
        {
            // Backslash escaped?
            if (*psz == '\\')
            {
                psz++;
                if (*psz)
                {
                    psz++;
                }
                continue;
            }

            // Quoted?
            if (*psz == '\"')
            {
                psz++;
                while (*psz && *psz != '\"')
                {
                    psz++;
                }
                if (*psz == '\"')
                    psz++;
                continue;
            }

            // End of arg?
            if (*psz == ' ' || *psz == '\t')
            {
                psz++;
                break;
            }

            psz++;
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
    while (*psz && pargs->argc < maxargc)
    {
        // Skip white space
        while (*psz == ' ' || *psz == '\t')
            psz++;

        // End of string
        if (!*psz)
            break;

        // Store start of string
        pargs->argv[pargs->argc] = pszDest;
        pargs->argc++;
        while (*psz)
        {
            // Backslash escaped?
            if (*psz == '\\')
            {
                psz++;
                if (*psz)
                {
                    *pszDest++ = *psz++;
                }
                continue;
            }

            // Quoted?
            if (*psz == '\"')
            {
                psz++;
                while (*psz && *psz != '\"')
                {
                    *pszDest++ = *psz++;
                }
                if (*psz == '\"')
                    psz++;
                continue;
            }

            // End of arg?
            if (*psz == ' ' || *psz == '\t')
            {
                *pszDest++ = '\0';
                psz++;
                break;
            }

            // Other char
            *pszDest++ = *psz++;
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

