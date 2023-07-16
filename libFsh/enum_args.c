#include "common.h"

#include "enum_args.h"

#include "path.h"
#include "ffex.h"

enum state
{
    state_init = 0,
    state_enum_opts = 1,
    state_enum_args = 2,
};

void start_enum_args(ENUM_ARGS* pctx, CMD_CONTEXT* pcmd, ARGS* pargs)
{
    pctx->pcmd = pcmd;
    pctx->pargs = pargs;
    pctx->index = 0;
    pctx->state = state_init;

    // Enum opts
    pctx->saveDelim = '\0';
    pctx->pszShort = NULL;

    // Enum args
    pctx->inFind = false;
    pctx->err = 0;
}


int handle_find_path(ENUM_ARGS* pctx, ARG* ppath)
{
    // Set in find flag
    pctx->inFind = true;

    // Work out absolute path
    strcpy(pctx->szAbs, pctx->sz);
    pathcat(pctx->szAbs, pctx->fi.fname);

    // Work out relative path
    if (pctx->pszRelBase)
    {
        size_t len = pctx->pszRelBase - pctx->pszArg;
        strncpy(pctx->szRel, pctx->pszArg, len);
        pctx->szRel[len] = '\0';
        pathcat(pctx->szRel, pctx->fi.fname);
    }
    else
    {
        strcpy(pctx->szRel, pctx->fi.fname);
    }

    // Setup result
    ppath->pszAbsolute = pctx->szAbs;
    ppath->pszRelative = pctx->szRel;
    ppath->pfi = &pctx->fi;
    return 0;
}


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

bool next_opt(ENUM_ARGS* pctx, OPT* popt)
{
    if (pctx->err)
        return false;

    // Restore delimiter
    if (pctx->saveDelim)
    {
        *pctx->delimPos = pctx->saveDelim;
        pctx->saveDelim = 0;
    }

    // Reset state?
    if (pctx->state != state_enum_opts)
    {
        pctx->pszShort = NULL;
        pctx->index = 0;
        pctx->state = state_enum_opts;
    }

decode_short:
    // Continued short name
    if (pctx->pszShort)
    {
        uint32_t cp = utf8_decode(&pctx->pszShort);
        if (cp)
        {
            // Copy character to temp buffer and terminate it
            pctx->sz[0] = '-';
            int len = utf8_encode(cp, pctx->sz + 1, 4);
            utf8_encode(0, pctx->sz + 1 + len, 4);

            // Get the char after
            const char* pcur = pctx->pszShort;
            uint32_t cpNext = utf8_decode(&pctx->pszShort);

            // Value
            if (cpNext == ':' || cpNext == '=')
            {
                popt->pszOpt = pctx->sz;
                popt->pszValue = pctx->pszShort;
                pctx->pszShort = NULL;
                return true;
            }
            else
            {
                popt->pszOpt = pctx->sz;
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

    pctx->state = state_init;
    return false;
}



bool is_opt(const char* pszOpt, const char* pszOptNames)
{
    const char* a = pszOpt;
    const char* b = pszOptNames;

    while (true)
    {
        // Decode one char from each
        uint32_t cpa = utf8_decode(&a);
        uint32_t cpb = utf8_decode(&b);

        // End of option?
        if (cpa == 0 && (cpb == 0 || cpb == '|'))
            return true;
        
        // Match?
        if (cpa == cpb)
            continue;
        
        // Mismatch, find next vbar
        while (true)
        {
            cpb = utf8_decode(&b);
            if (cpb == 0)
                return false;
            if (cpb == '|')
                break;
        }

        // Reset to start of opt
        a = pszOpt;
    }
}


bool next_arg(ENUM_ARGS* pctx, ARG* ppath)
{
    // For perr
    CMD_CONTEXT* pcmd = pctx->pcmd;

    // Reset state?
    if (pctx->state != state_enum_args)
    {
        pctx->index = 0;
        pctx->state = state_enum_args;
    }

    // Continue find?
    if (pctx->inFind)
    {
        // Find next
        int err = f_findnext(&pctx->dir, &pctx->fi);
        while (!err && pctx->fi.fname[0] && f_is_hidden(&pctx->fi))
            err = f_findnext(&pctx->dir, &pctx->fi);
        if (err != FR_OK)
        {
            perr("find path failed: '%s', %s (%i)\n", pctx->pszArg, f_strerror(err), err);
            abort_enum_args(pctx, f_maperr(err));
            return false;
        }
        
        // Handle it if more
        if (pctx->fi.fname[0])
        {
            handle_find_path(pctx, ppath);
            return true;
        }
        
        // No longer in find
        f_closedir(&pctx->dir);
        pctx->inFind = false;
    }

    // Process all args
    while (pctx->index < pctx->pargs->argc)
    {
        // Get next arg
        pctx->pszArg = pctx->pargs->argv[pctx->index++];

        // Get full path
        strcpy(pctx->sz, pctx->pcmd->cwd);
        pathcat(pctx->sz, pctx->pszArg);
        pathcan(pctx->sz);
        if (pathisdir(pctx->pszArg))
            pathensuredir(pctx->sz); // Maintain trailing slash

        // Split the full path at last element
        pctx->pszBase = pathsplit(pctx->sz);
        pctx->pszRelBase = pathbase(pctx->pszArg);

        // Check no wildcard was in the directory part
        if (pathiswild(pctx->sz))
        {
            perr("wildcards in directory paths not supported: '%s'\n", pctx->sz);
            abort_enum_args(pctx, -1);
            return false;
        }

        // Does it contain wildcards
        if (pathiswild(pctx->pszBase))
        {
            // Enumerate directory
            int err = f_findfirst(&pctx->dir, &pctx->fi, pctx->sz, pctx->pszBase);
            while (!err && pctx->fi.fname[0] && f_is_hidden(&pctx->fi))
                err = f_findnext(&pctx->dir, &pctx->fi);
            if (err)
            {
                perr("find path failed: '%s', %s (%i)\n", pctx->pszArg, f_strerror(err), err);
                abort_enum_args(f_maperr(err));
                return false;
            }
            if (pctx->fi.fname[0])
            {
                handle_find_path(pctx, ppath);
                return true;
            }
            
            f_closedir(&pctx->dir);
        }
        else
        {
            // Setup PATHINFO
            ((char*)pctx->pszBase)[-1] = '/';      // unsplit
            ppath->pszRelative = pctx->pszArg;
            ppath->pszAbsolute = pctx->sz;
            ppath->pfi = NULL;

            // Is it the root directory

            // Stat
            if (f_stat_ex(pctx->sz, &pctx->fi) == FR_OK)
            {
                ppath->pfi = &pctx->fi;

                // If path looks like a directory, but matched a file
                // then mark it as not found
                if (pathisdir(ppath->pszAbsolute) && (pctx->fi.fattrib & AM_DIR)==0)
                {
                    perr("path is not a directory: '%s'\n", ppath->pszRelative);
                    abort_enum_args(pctx, -1);
                    return false;
                }
            }

            return true;
        }
    }

    // End
    ppath->pfi = NULL;
    ppath->pszAbsolute = NULL;
    ppath->pszRelative = NULL;
    return false;
}

int end_enum_args(ENUM_ARGS* pctx)
{
    if (pctx->inFind)
    {
        f_closedir(&pctx->dir);
        pctx->inFind = false;
    }
    return pctx->err;
}

void abort_enum_args(ENUM_ARGS* pctx, int err)
{
    // Set error code
    set_enum_args_error(pctx, err);

    // Abort find file
    if (pctx->inFind)
    {
        f_closedir(&pctx->dir);
        pctx->inFind = false;
    }

    // Move index to end
    pctx->index = pctx->pargs->argc;
}

// Set error code to be returned from end_enum_args, but continue enumeration
void set_enum_args_error(ENUM_ARGS* pctx, int err)
{
    // Record error (unless another error already set)
    if (pctx->err == 0)
        pctx->err = err;
}


