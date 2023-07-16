#include "common.h"

#include "enum_opts.h"
#include "utf.h"

const char* find_delim(const char* psz)
{
    while (true)
    {
        const char* pprev = psz;
        uint32_t cp = utf8_decode(&psz);
        if (cp == ':' || cp == '=')
            return pprev;
        if (cp == 0)
            return NULL;
    }
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

decode_short:
    // Continued short name
    if (pctx->pszShort)
    {
        uint32_t cp = utf8_decode(&pctx->pszShort);
        if (cp)
        {
            // Copy character to temp buffer and terminate it
            int len = utf8_encode(cp, pctx->szTemp + 1, 4);
            utf8_encode(0, pctx->szTemp + 1 + len, 4);

            // Get the char after
            const char* pcur = pctx->pszShort;
            uint32_t cpNext = utf8_decode(&pctx->pszShort);

            // Value
            if (cpNext == ':' || cpNext == '=')
            {
                popt->pszOpt = pctx->szTemp;
                popt->pszValue = pctx->pszShort;
                pctx->pszShort = NULL;
                return true;
            }
            else
            {
                popt->pszOpt = pctx->szTemp;
                popt->pszValue = NULL;
                pctx->pszShort = pcur;
                return true;
            }
        }

        pctx->pszShort = NULL;
    }

    while (pctx->index < pctx->pargs->argc)
    {
        const char* pszArg = pctx->pargs->argv[pctx->index++];
        const char* p = pszArg;

        // Switch?
        uint32_t cp1 = utf8_decode(&p);
        if (cp1 != '-')
            continue;

        // Remove the argument
        remove_arg(pctx->pargs, pctx->index-1);
        pctx->index--;

        // Decode the second character
        const char* pszShort = p;
        uint32_t cp2 = utf8_decode(&p);

        if (cp2 == '-')
        {
            // Long switch

            // Find '=' || ':' value separator
            char* pszDelim = (char*)find_delim(pszArg);
            if (pszDelim)
            {
                // Setup return
                pctx->saveDelim = *pszDelim;
                pctx->delimPos = pszDelim;
                popt->pszOpt = pszArg;
                popt->pszValue = pszDelim;
                utf8_decode(&popt->pszValue);
                *pszDelim = '\0';
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
            pctx->pszShort = pszShort;
            goto decode_short;
        }
    }
    return false;
}

