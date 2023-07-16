#include <string.h>

#include "enum_args.h"

#include "path.h"
#include "ffex.h"

void start_enum_args(ENUM_ARGS* pctx, CMD_CONTEXT* pcmd, ARGS* pargs)
{
    pctx->pcmd = pcmd;
    pctx->pargs = pargs;
    pctx->index = 0;
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



bool next_arg(ENUM_ARGS* pctx, ARG* ppath)
{
    if (pctx->err)
        return false;

    // For perr
    CMD_CONTEXT* pcmd = pctx->pcmd;

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
            pctx->err = f_maperr(err);
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
            pctx->err = -1;
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
                pctx->err = f_maperr(err);
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
                    pctx->err = -1;
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


