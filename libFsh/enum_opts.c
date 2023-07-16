#include <string.h>

#include "enum_opts.h"

const char* find_delim(const char* psz)
{
    while (*psz)
    {
        if (*psz == ':' || *psz == '=')
            return psz;
        psz++;
    }
    return NULL;
}

void enum_opts(ENUM_OPTS* pctx, ARGS* pargs)
{
    pctx->pargs = pargs;
    pctx->index = 0;
    pctx->saveDelim = '\0';
    strcpy(pctx->szTemp, "-?");
    pctx->pszShort = NULL;
}

bool next_opt(ENUM_OPTS* pctx, OPT* popt)
{
    // Restore delimiter
    if (pctx->saveDelim)
    {
        *pctx->delimPos = pctx->saveDelim;
        pctx->saveDelim = 0;
    }

    // Continued short name
    if (pctx->pszShort)
    {
        if (pctx->pszShort[1] == ':' || pctx->pszShort[1] == '=')
        {
            pctx->szTemp[1] = *pctx->pszShort;
            popt->pszOpt = pctx->szTemp;
            popt->pszValue = pctx->pszShort+1;
            pctx->pszShort = NULL;
            return true;
        }

        if (*pctx->pszShort)
        {
            pctx->szTemp[1] = *pctx->pszShort++;
            popt->pszOpt = pctx->szTemp;
            popt->pszValue = NULL;
            return true;
        }

        pctx->pszShort = NULL;
    }

    while (pctx->index < pctx->pargs->argc)
    {
        const char* pszArg = pctx->pargs->argv[pctx->index++];

        // Switch?
        if (pszArg[0] != '-')
            continue;

        // Remove the argument
        remove_arg(pctx->pargs, pctx->index-1);
        pctx->index--;

        if (pszArg[1] == '-')
        {
            // Long switch

            // Find '=' || ':' value separator
            char* pszDelim = (char*)find_delim(pszArg);
            if (pszDelim)
            {
                // Setup return
                pctx->saveDelim = *pszDelim;
                pctx->delimPos = pszDelim;
                *pszDelim = '\0';
                popt->pszOpt = pszArg;
                popt->pszValue = pszDelim + 1;
                return true;
            }
            else
            {
                popt->pszOpt = pszArg;
                popt->pszValue = NULL;
                return true;
            }
        }
        else
        {
            pctx->pszShort = pszArg + 1;
            pctx->szTemp[1] = *pctx->pszShort++;
            popt->pszOpt = pctx->szTemp;
            if (*pctx->pszShort == ':' || *pctx->pszShort == '=')
            {
                popt->pszValue = pctx->pszShort+1;
                pctx->pszShort = NULL;
            }
            else
            {
                popt->pszValue = NULL;
            }
            return true;
        }
    }
    return false;
}

