#include "common.h"

#include "enum_args.h"

#include "path.h"
#include "ffex.h"
#include "utf.h"
#include "bracexp.h"
#include "specialchars.h"
#include "process.h"

enum state
{
    state_init = 0,
    state_enum_opts = 1,
    state_enum_args = 2,
    state_readdir = 3,
    state_expand_braces = 4,
    state_return_arg = 5,
    state_process_arg = 6,
};

void start_enum_args(ENUM_ARGS* pctx, struct PROCESS* proc, struct ARGS* pargs)
{
    pctx->proc = proc;
    pctx->pargs = pargs;
    pctx->index = 0;
    pctx->state = state_init;

    pctx->bracePerm = -100;

    // Enum opts
    pctx->saveDelim = '\0';
    pctx->pszShort = NULL;

    // Enum args
    pctx->err = 0;
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
            int len = utf8_encode_ex(cp, pctx->sz + 1, 4);
            utf8_encode_ex(0, pctx->sz + 1 + len, 4);

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

// Check if popt matches opt name, and no value
bool is_switch(ENUM_ARGS* pctx, OPT* popt, const char* pszOptNames)
{
    if (!is_opt(popt->pszOpt, pszOptNames))
        return false;

    if (popt->pszValue)
    {
        struct PROCESS* proc = pctx->proc;
        perr("unexpected value passed to '%s'", popt->pszOpt);
        set_enum_args_error(pctx, -1);
        return false;
    }
    return true;
}

// Check if popt matches opt name, with value
bool is_value_opt(ENUM_ARGS* pctx, OPT* popt, const char* pszOptNames)
{
    if (!is_opt(popt->pszOpt, pszOptNames))
        return false;

    if (!popt->pszValue)
    {
        struct PROCESS* proc = pctx->proc;
        perr("expected value for '%s'", popt->pszOpt);
        set_enum_args_error(pctx, -1);
        return false;
    }

    return true;
}

// Unknown opt handler
void unknown_opt(ENUM_ARGS* pctx, OPT* popt)
{
    struct PROCESS* proc = pctx->proc;
    perr("invalid option: '%s'", popt->pszOpt);
    abort_enum_args(pctx, -1);
}

static bool direntry_filter(void* user, FILINFO* pfi)
{
    // Always ignore hidden
    if (f_is_hidden(pfi))
        return false;

    // Does it match pattern?
    if (!pathglob(pfi->fname, (const char*)user, false, special_chars))
        return false;

    return true;
}



bool next_arg(ENUM_ARGS* pctx, ARG* ppath)
{
    // For perr
    struct PROCESS* proc = pctx->proc;

    while (true)
    {
        switch (pctx->state)
        {
            case state_enum_args:
            {
                // Any more?
                if (pctx->index >= pctx->pargs->argc)
                    return false;

                // Get next arg
                const char* pszArg = pctx->pargs->argv[pctx->index++];

                // Prepare brace expansion
                pctx->bracePerms = bracexp_prepare(pszArg, special_chars, pctx->braceOps, sizeof(pctx->braceOps));
                if (pctx->bracePerms < 0)
                {
                    perr("brace expansion expression too complex");
                    abort_enum_args(pctx, -1);
                    break;
                }

                // Setup brace perm enumeration
                pctx->bracePerm = 0;
                if (pctx->bracePerms == 0)
                {
                    strcpy(pctx->szArg, pszArg);
                    restore_brace_special_chars(pctx->szArg);
                    pctx->state = state_process_arg;
                }
                else
                {
                    pctx->state = state_expand_braces;
                }
                break;

            case state_expand_braces:
                // End of enumeration?
                if (pctx->bracePerm >= pctx->bracePerms)
                {
                    pctx->state = state_enum_args;
                    break;
                }

                // Expand next perm
                bracexp_expand(pctx->szArg, pctx->bracePerm++, pctx->braceOps);
                pctx->state = state_process_arg;
                break;

            case state_process_arg:
                // Get full path
                strcpy(pctx->sz, pctx->proc->cwd);
                pathcat(pctx->sz, pctx->szArg);
                pathcan(pctx->sz);
                if (pathisdir(pctx->szArg))
                    pathensuredir(pctx->sz); // Maintain trailing slash

                // Split the full path at last element
                pctx->pszBase = pathsplit(pctx->sz);
                pctx->pszRelBase = pathbase(pctx->szArg);

                // Check no wildcard in the directory part
                if (pathisglob(pctx->sz, special_chars))
                {
                    perr("wildcards in directory paths not supported: '%s'\n", pctx->sz);
                    abort_enum_args(pctx, -1);
                    return false;
                }

                // Does it contain wildcards
                if (pathisglob(pctx->pszBase, special_chars))
                {
                    // Enumerate directory
                    pctx->didMatch = false;
                    int err = f_opendir_ex(&pctx->direx, pctx->sz, (void*)pctx->pszBase, direntry_filter, NULL, direntry_compare_name);
                    if (err)
                    {
                        perr("find path failed: '%s', %s (%i)\n", pctx->szArg, f_strerror(err), err);
                        abort_enum_args(pctx, f_maperr(err));
                        return false;
                    }
                    pctx->state = state_readdir;
                    break;
                }
                else
                {
                    pctx->state = state_return_arg;
                }
                break;

            case state_return_arg:

                // Setup PATHINFO
                ((char*)pctx->pszBase)[-1] = '/';      // unsplit
                ppath->pszRelative = pctx->szArg;
                ppath->pszAbsolute = pctx->sz;
                ppath->pfi = NULL;

                // Stat
                FILINFO fi;
                if (f_stat_ex(pctx->sz, &fi) == FR_OK)
                {
                    // If path looks like a directory, but matched a file
                    // then mark it as not found
                    if (pathisdir(ppath->pszAbsolute) && (fi.fattrib & AM_DIR)==0)
                    {
                        perr("path is not a directory: '%s'\n", ppath->pszRelative);
                        abort_enum_args(pctx, -1);
                        return false;
                    }

                    ppath->pfi = direntry_alloc(&pctx->proc->pool, &fi);
                }

                pctx->state = state_expand_braces;
                return true;
            }

            case state_readdir:
            {
                // Read next directory entry
                DIRENTRY* pde;
                if (!f_readdir_ex(&pctx->direx, &pde))
                {
                    // Close dir
                    f_closedir_ex(&pctx->direx);

                    // If didn't match any files
                    if (!pctx->didMatch)
                    {
                        // Return the glob pattern as an argument
                        restore_glob_special_chars(pctx->szArg);
                        pctx->state = state_return_arg;
                    }
                    else
                    {
                        pctx->state = state_expand_braces;
                    }

                    break;
                }

                // Remember we matched at least one file
                pctx->didMatch = true;

                // Work out absolute path
                strcpy(pctx->szAbs, pctx->sz);
                pathcat(pctx->szAbs, pde->fname);

                // Work out relative path
                if (pctx->pszRelBase)
                {
                    size_t len = pctx->pszRelBase - pctx->szArg;
                    strncpy(pctx->szRel, pctx->szArg, len);
                    pctx->szRel[len] = '\0';
                    pathcat(pctx->szRel, pde->fname);
                }
                else
                {
                    strcpy(pctx->szRel, pde->fname);
                }

                // Setup result
                ppath->pszAbsolute = pctx->szAbs;
                ppath->pszRelative = pctx->szRel;
                ppath->pfi = pde;
                return true;
            }

            default:
                pctx->state = state_enum_args;
                pctx->index = 0;
                break;
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
    if (pctx->state == state_readdir)
    {
        f_closedir_ex(&pctx->direx);
        pctx->state = state_init;
    }
    return pctx->err;
}

int enum_args_error(ENUM_ARGS* pctx)
{
    if (pctx->err)
        end_enum_args(pctx);

    return pctx->err;
}


void abort_enum_args(ENUM_ARGS* pctx, int err)
{
    // Set error code
    set_enum_args_error(pctx, err);

    // Abort find file
    if (pctx->state == state_readdir)
    {
        f_closedir_ex(&pctx->direx);
        pctx->state = state_enum_args;
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


