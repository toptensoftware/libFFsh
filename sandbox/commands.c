#include "common.h"
#include <stdarg.h>
#include <errno.h>

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
    memmove(pargs->argv + position, pargs->argv + position + 1, nShift * sizeof(const char*));

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


const char* f_strerror(int err)
{
    switch (err)
    {
        case FR_OK: return "Succeeded";
        case FR_DISK_ERR: return "a hard error occurred in the low level disk I/O layer";
        case FR_INT_ERR: return "assertion failed";
        case FR_NOT_READY: return "the physical drive cannot work";
        case FR_NO_FILE: return "could not find the file";
        case FR_NO_PATH: return "could not find the path";
        case FR_INVALID_NAME: return "the path name format is invalid";
        case FR_DENIED: return "access denied due to prohibited access or directory full";
        case FR_EXIST: return "access denied due to prohibited access";
        case FR_INVALID_OBJECT: return "the file/directory object is invalid";
        case FR_WRITE_PROTECTED: return "the physical drive is write protected";
        case FR_INVALID_DRIVE: return "the logical drive number is invalid";
        case FR_NOT_ENABLED: return "the volume has no work area";
        case FR_NO_FILESYSTEM: return "there is no valid FAT volume";
        case FR_MKFS_ABORTED: return "the f_mkfs() aborted due to any problem";
        case FR_TIMEOUT: return "could not get a grant to access the volume within defined period";
        case FR_LOCKED: return "the operation is rejected according to the file sharing policy";
        case FR_NOT_ENOUGH_CORE: return "LFN working buffer could not be allocated";
        case FR_TOO_MANY_OPEN_FILES: return "too many open files";
        case FR_INVALID_PARAMETER: return "given parameter is invalid";
    }
    return "unknown error";
}

// Map a FatFS error code to standard error code
int f_maperr(int err)
{
    return err + 1000;
}

// Print to stderr
void perr(const char* format, ...)
{
	va_list args;
	va_start(args, format);
    char szTemp[FF_MAX_LFN + 100];
    vsnprintf(szTemp, sizeof(szTemp), format, args);
    fputs(szTemp, stderr);
    va_end(args);
}

// Print to stdout
void pout(const char* format, ...)
{
	va_list args;
	va_start(args, format);
    char szTemp[FF_MAX_LFN + 100];
    vsnprintf(szTemp, sizeof(szTemp), format, args);
    fputs(szTemp, stdout);
    va_end(args);
}


void start_enum_args(ENUM_ARGS* pctx, const char* cwd, ARGS* pargs)
{
    pctx->cwd = cwd;
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

FRESULT f_stat_ex(
    const TCHAR* path,	/* Pointer to the file path */
	FILINFO* fno		/* Pointer to file information to return */
)
{
    if (pathisroot(path))
    {
        fno->fdate = 0;
        fno->ftime = 0;
        fno->fsize = 0;
        fno->fattrib = AM_DIR;
        strcpy(fno->fname, "/");
        return FR_OK;
    }

    return f_stat(path, fno);
}

bool is_hidden(FILINFO* pfi)
{
    return (pfi->fattrib & AM_HID) || (pfi->fname[0] == '.');
}


bool next_arg(ENUM_ARGS* pctx, ARG* ppath)
{
    if (pctx->err)
        return false;

    // Continue find?
    if (pctx->inFind)
    {
        // Find next
        int err = f_findnext(&pctx->dir, &pctx->fi);
        while (!err && pctx->fi.fname[0] && is_hidden(&pctx->fi))
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
        strcpy(pctx->sz, pctx->cwd);
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
            pctx->err = ENOENT;
            return false;
        }

        // Does it contain wildcards
        if (pathiswild(pctx->pszBase))
        {
            // Enumerate directory
            int err = f_findfirst(&pctx->dir, &pctx->fi, pctx->sz, pctx->pszBase);
            while (!err && pctx->fi.fname[0] && is_hidden(&pctx->fi))
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




int cmd_touch(CMD_CONTEXT* pctx)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("touch: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    // Process args
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        if (!arg.pfi)
        {
            // Not if looks like a directory
            if (pathisdir(arg.pszRelative))
            {
                perr("touch: no such directory '%s'\n", arg.pszRelative);
                set_enum_args_error(&args, ENOENT);
                continue;
            }

            // Create new file
            FIL f;
            int err = f_open(&f, arg.pszAbsolute, FA_CREATE_NEW | FA_WRITE);
            if (err)
            {
                perr("touch: failed to create file '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
                continue;
            }
            f_close(&f);
        }
        else
        {
            // Touch existing file/directory
            FILINFO fno;
            DWORD dwTime = get_fattime();
            fno.fdate = dwTime >> 16;
            fno.ftime = dwTime & 0xFFFF;
            int err = f_utime(arg.pszAbsolute, &fno);
            if (err)
            {
                perr("touch: failed to update times: '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
                continue;
            }
        }
    }
    return end_enum_args(&args);
}

int cmd_ls_item(const char* pszRelative, FILINFO* pfi, bool optLong)
{
//    if (pszRelative[0] == '.' && (pszRelative[1] == '\\' || pszRelative[1] == '/'))
//        pszRelative += 2;
    if (optLong)
    {
        pout("%c%c%c%c%c %02i/%02i/%04i %02i:%02i:%02i %12i %s\n", 
            (pfi->fattrib & AM_DIR) ? 'd' : '-',
            (pfi->fattrib & AM_RDO) ? 'r' : '-',
            (pfi->fattrib & AM_SYS) ? 's' : '-',
            (pfi->fattrib & AM_HID) ? 'h' : '-',
            (pfi->fattrib & AM_ARC) ? 'a' : '-',
            (pfi->fdate & 0x1f),
            ((pfi->fdate >> 5) & 0x0f),
            ((pfi->fdate >> 9) & 0x7f) + 1980,
            ((pfi->ftime >> 11) & 0x1f),
            ((pfi->ftime >> 5) & 0x3f),
            ((pfi->ftime >> 0) & 0x1f) * 2,
            pfi->fsize,
            pszRelative);
    }
    else
    {
        pout("%s  ", pszRelative);
    }

    return 0;
}

int cmd_ls_dir(const char* pszAbsolute, const char* pszRelative, bool optAll, bool optLong)
{
    DIR dir;
    int err = f_opendir(&dir, pszAbsolute);
    if (err)
    {
        perr("ls: failed to opendir: '%s', %s (%i)", pszRelative, f_strerror(err), err);
        return f_maperr(err);
    }

    FILINFO fi;
    bool anyItems = false;
    while (true)
    {
        int err = f_readdir(&dir, &fi);
        if (err)
        {
            perr("ls: failed to readdir: '%s', %s (%i)", pszRelative, f_strerror(err), err);
            f_closedir(&dir);
            return f_maperr(err);
        }
        if (fi.fname[0] == '\0')
            break;

        if (optAll || !is_hidden(&fi))
        {
            cmd_ls_item(fi.fname, &fi, optLong);
            anyItems = true;
        }
    }

    if (anyItems && !optLong)
        printf("\n");

    f_closedir(&dir);
    return 0;
}

int cmd_ls(CMD_CONTEXT* pctx)
{
    bool optAll = false;
    bool optLong = false;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-a") == 0)  
            optAll = true;
        else if (strcmp(opt.pszOpt, "-l") == 0)
            optLong = true;
        else
        {
            perr("ls: unknown option: '%s'\n", opt.pszOpt);
            return -1;
        }
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    bool anydirs = false;
    bool anyargs = false;
    bool anyitems = false;
    while (next_arg(&args, &arg))
    {
        anyargs = true;
        if (!arg.pfi)
        {
            perr("ls: no such file or directory: '%s'\n", arg.pszRelative);
        }
        else
        {
            if ((arg.pfi->fattrib & AM_DIR) == 0)
            {
                int err = cmd_ls_item(arg.pszRelative, arg.pfi, optLong);
                if (err)
                    set_enum_args_error(&args, err);
                anyitems = true;
            }
            else
            {
                anydirs = true;
            }
        }
    }
    if (anyitems && !optLong)
        printf("\n");
    int result = end_enum_args(&args);

    if (anydirs)
    {
        // Process args (2nd pass list directories)
        start_enum_args(&args, pctx->cwd, pctx->pargs);
        set_enum_args_error(&args, result);
        bool first = !anyitems;
        while (next_arg(&args, &arg))
        {
            if (arg.pfi && arg.pfi->fattrib & AM_DIR)
            {
                if (!first)
                    pout("\n");
                else
                    first = false;
                pout("%s:\n", arg.pszRelative);
                int err = cmd_ls_dir(arg.pszAbsolute, arg.pszRelative, optAll, optLong);
                if (err)
                    set_enum_args_error(&args, err);
            }
        }
        result = end_enum_args(&args);        
    }

    if (!anyargs)
    {
        result = cmd_ls_dir(pctx->cwd, ".", optAll, optLong);
    }

    return result;
}

// Recursively remove a directory
// If psz is a folder, then the buffer should
// be at least FF_MAX_LFN in size
int f_rmdir_r(char* psz)
{
    // Otherwise enumerate the directory, deleting all sub items
    DIR dir;
    int err = f_opendir(&dir, psz);
    if (err)
        return err;

    // Remember end of passed path
    char* pszEnd = psz + strlen(psz);
    bool dirOpen = true;

    // Delete all directory entries
    while (true)
    {
        FILINFO fi;
        int err = f_readdir(&dir, &fi);
        if (err)
        {
            f_closedir(&dir);
            return err;
        }

        // Nothing more?
        if (fi.fname[0] == '\0')
            break;

        // Get full name
        pathcat(psz, fi.fname);

        // Delete file, recurse sub-directory
        if (fi.fattrib & AM_DIR)
        {
            err = f_rmdir_r(psz);

            // If too many files error, then close our
            // directory, remove the sub-directory then
            // re-open
            if (err == FR_TOO_MANY_OPEN_FILES)
            {
                f_closedir(&dir);
                dirOpen = false;

                err = f_rmdir_r(psz);

                if (err == 0)
                {
                    *pszEnd = '\0';
                    err = f_opendir(&dir, psz);
                    dirOpen = err == 0;
                }
            }
        }
        else
        {
            err = f_unlink(psz);
        }

        *pszEnd = '\0';

        if (err)
        {
            if (dirOpen)
                f_closedir(&dir);
            return err;
        }
    }

    if (dirOpen)
        f_closedir(&dir);

    // Remove the now empty directory
    return f_rmdir(psz);
}



int cmd_rm(CMD_CONTEXT* pctx)
{
    bool optRecursive = false;
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-r") == 0)
        {
            optRecursive = true;
            continue;
        }

        perr("rm: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        if (!arg.pfi)
        {
            perr("rm: no such file or directory: '%s'\n", arg.pszRelative);
        }
        else
        {
            if ((arg.pfi->fattrib & AM_DIR) == 0)
            {
                int err = f_unlink(arg.pszAbsolute);
                if (err)
                {
                    perr("rm: error removing '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                    set_enum_args_error(&args, f_maperr(err));;
                }
            }
            else
            {
                if (optRecursive)
                {
                    int err = f_rmdir_r((char*)arg.pszAbsolute);
                    if (err)
                    {
                        perr("rm: error removing '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                        set_enum_args_error(&args, f_maperr(err));;
                    }
                }
                else
                {
                    perr("rm: cannot remove directory '%s'\n", arg.pszRelative);
                    set_enum_args_error(&args, -1);
                }
            }
        }
    }
    return end_enum_args(&args);
}

int f_mkdir_r(const char* psz)
{
    // Is it the root directory
    if (pathisroot(psz))
        return 0;

    // Try to create it
    int err = f_mkdir(psz);
    if (err == 0 || err != FR_NO_PATH)
        return 0;

    // Find base
    char* base = (char*)pathbase(psz);
    if (!base)
        return ENOENT;

    // Make parent
    char chSave = base[-1];
    base[-1] = '\0';
    err = f_mkdir_r(psz);
    base[-1] = chSave;
    if (err)
        return err;

    // Try again
    return f_mkdir(psz);
}

int cmd_mkdir(CMD_CONTEXT* pctx)
{
    bool makeParents = false;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-p") == 0 || strcmp(opt.pszOpt, "--parents") == 0)
        {
            if (opt.pszValue == NULL)
            {
                makeParents = true;
                continue;
            }
            else
            {
                perr("mkdir: unexpected value '%s'\n", opt.pszValue);
                return -1;
            }
        }

        perr("mkdir: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        if (!arg.pfi)
        {
            int err;
            if (makeParents)
                err = f_mkdir_r(arg.pszAbsolute);
            else
                err = f_mkdir(arg.pszAbsolute);
            if (err)
            {
                perr("mkdir: error '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
            }
        }
        else
        {
            if (!makeParents)
                perr("mkdir: already exists: '%s'\n", arg.pszRelative);
        }
    }
    return end_enum_args(&args);
}

int cmd_cd(CMD_CONTEXT* pctx)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("cd: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    char newcwd[FF_MAX_LFN];
    newcwd[0] = '\0';

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        if (!arg.pfi)
        {
            perr("cd: path not found: '%s'\n", arg.pszRelative);
            abort_enum_args(&args, ENOENT);
        }
        else
        {
            if (arg.pfi->fattrib & AM_DIR)
            {
                if (newcwd[0] == '\0')
                {
                    strcpy(newcwd, arg.pszAbsolute);
                }
                else
                {
                    perr("cd: too many arguments: '%s'\n", arg.pszRelative);
                    abort_enum_args(&args, -1);
                }
            }
        }
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    strcpy(pctx->cwd, newcwd);
    return 0;
}

int cmd_pwd(CMD_CONTEXT* pctx)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("cd: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        perr("pwd: too many arguments: '%s'\n", arg.pszRelative);
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    pout("%s\n", pctx->cwd);
    return 0;
}

int cmd_rmdir(CMD_CONTEXT* pctx)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("rmdir: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        int err = f_rmdir(arg.pszAbsolute);
        if (err)
        {
            const char* pszErr = f_strerror(err);
            if (err == FR_DENIED)
                pszErr = "directory not empty";
            perr("rmdir: error '%s', %s (%i)\n", arg.pszRelative, pszErr, err);
            set_enum_args_error(&args, f_maperr(err));
        }
    }
    return end_enum_args(&args);
}

int cmd_echo(CMD_CONTEXT* pctx)
{
    char szOutFile[FF_MAX_LFN];
    bool optOut = false;
    bool optAppend = false;
    bool optNoNewLine = false;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-o") == 0 || strcmp(opt.pszOpt, "--out") == 0)
        {
            if (optOut)
            {
                perr("echo: error: multiple -o options");
                return -1;
            }
            if (opt.pszValue == NULL)
            {
                perr("echo: error: -o argument missing\n");
                return -1;
            }
            if (pathisdir(opt.pszValue))
            {
                perr("echo: error: bad file name: '%s'\n", opt.pszValue);
            }
            strcpy(szOutFile, pctx->cwd);
            pathcat(szOutFile, opt.pszValue);
            pathcan(szOutFile);
            optOut = true;
        }
        else if (strcmp(opt.pszOpt, "-a") == 0 || strcmp(opt.pszOpt, "--append") == 0)
        {
            optAppend = true;
        }
        else if (strcmp(opt.pszOpt, "-n") == 0)
        {
            optNoNewLine = true;
        }
        else
        {
            perr("echo: unknown option: '%s'\n", opt.pszOpt);
            return -1;
        }
    }

    // Open output file
    FIL file;
    if (optOut)
    {
        int err = f_open(&file, szOutFile, optAppend ? (FA_CREATE_NEW | FA_WRITE) : (FA_OPEN_ALWAYS | FA_OPEN_APPEND | FA_WRITE));
        if (err)
        {
            perr("echo: error: open file failed: '%s', %s (%i)\n", szOutFile, f_strerror(err), err);
            return err;
        }
    }

    // Process args
    ENUM_ARGS args;
    ARG arg;
    bool first = true;
    UINT unused;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        if (optOut)
        {
            if (!first)
                f_write(&file, " ", 1, &unused);
            int err = f_write(&file, arg.pszRelative, strlen(arg.pszRelative), &unused);
            if (err)
            {
                perr("echo: failed to write: '%s', %s (%i)\n", szOutFile, f_strerror(err), err);
                f_close(&file);
                return f_maperr(err);
            }
        }
        else
        {
            if (!first)
                pout(" ");
            pout("%s", arg.pszRelative);
        }
        first = false;
    }

    if (!optNoNewLine)
    {
        if (optOut)
        {
            f_write(&file, "\n", 1, &unused);
        }
        else
        {
            pout("\n");
        }
    }


    if (optOut)
    {
        f_close(&file);
    }

    return end_enum_args(&args);
}

int cmd_cat(CMD_CONTEXT* pctx)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("cat: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    // Process args
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        if (arg.pfi == NULL)
        {
            perr("cat: file not found: '%s'\n", arg.pszRelative);
        }
        else
        {
            if (arg.pfi->fattrib & AM_DIR)
            {
                perr("cat: '%s' is a directory\n", arg.pszRelative);
            }
            else
            {
                // Open file
                FIL file;
                int err = f_open(&file, arg.pszAbsolute, FA_OPEN_EXISTING | FA_READ);
                if (err)
                {
                    perr("cat: error opening file '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                    set_enum_args_error(&args, f_maperr(err));
                    continue;
                }

                // Read it
                char sz[129];
                while (true)
                {
                    // Read next buffer
                    UINT bytes_read;
                    err = f_read(&file, sz, sizeof(sz) - 1, &bytes_read);
                    if (err)
                    {
                        perr("cat: error reading file '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                        set_enum_args_error(&args, f_maperr(err));
                        break;
                    }

                    // Print it
                    sz[bytes_read] = '\0';
                    pout("%s", sz);

                    if (bytes_read < sizeof(sz) -1)
                        break;
                }

                f_close(&file);
            }
        }
    }

    return end_enum_args(&args);
}

void hexdump(uint32_t offset, const char* buf, int len)
{
    pout("%08x: ", offset);

    for (int i=0; i<len; i++)
    {
        if (i == 8)
            pout(" ");
        pout("%02x ", buf[i]);
    }

    for (int i=len; i < 16; i++)
    {
        if (i == 8)
            pout(" ");
        pout("   ");
    }

    pout("| ");

    for (int i=0; i<len; i++)
    {
        if (buf[i] >= 32 && buf[i] <= 0x7f)
        {
            pout("%c", buf[i]);
        }
        else
        {
            pout(".");
        }
    }

    pout("\n");
}

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

int cmd_hexdump(CMD_CONTEXT* pctx)
{
    uint32_t optSkip = 0;
    uint32_t optLength = 0xFFFFFFFF;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-s") == 0 || strcmp(opt.pszOpt, "--skip") == 0)
        {
            if (opt.pszValue == NULL)
            {
                perr("hexdump: error: --skip argument missing\n");
                return -1;
            }
            else
            {   
                if (!parse_uint32(opt.pszValue, &optSkip))
                {
                    perr("hexdump: error: invalid --skip argument '%s'\n", opt.pszValue);
                    return -1;
                }
            }
        }
        else if (strcmp(opt.pszOpt, "-n") == 0 || strcmp(opt.pszOpt, "--length") == 0)
        {
            if (opt.pszValue == NULL)
            {
                perr("hexdump: error: --length argument missing\n");
                return -1;
            }
            else
            {   
                if (!parse_uint32(opt.pszValue, &optLength))
                {
                    perr("hexdump: error: invalid --length argument '%s'\n", opt.pszValue);
                    return -1;
                }
            }
        }
        else
        {
            perr("hexdump: unknown option: '%s'\n", opt.pszOpt);
            return -1;
        }
    }

    // Read buffer
    char buf[128];
    int bufUsed = 0;
    uint32_t offset = 0;

    // Process args
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg) && offset < optSkip + optLength)
    {
        if (arg.pfi == NULL)
        {
            perr("hexdump: file not found: '%s'\n", arg.pszRelative);
        }
        else
        {
            if (arg.pfi->fattrib & AM_DIR)
            {
                perr("hexdump: '%s' is a directory\n", arg.pszRelative);
            }
            else
            {
                // Open file
                FIL file;
                int err = f_open(&file, arg.pszAbsolute, FA_OPEN_EXISTING | FA_READ);
                if (err)
                {
                    perr("hexdump: error opening file '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                    set_enum_args_error(&args, f_maperr(err));
                    continue;
                }

                // Read it
                while (true)
                {
                    // Read next buffer
                    UINT bytes_read;
                    err = f_read(&file, buf + bufUsed, sizeof(buf) - bufUsed, &bytes_read);
                    if (err)
                    {
                        perr("hexdump: error reading file '%s', %s (%i)\n", arg.pszRelative, f_strerror(err), err);
                        set_enum_args_error(&args, f_maperr(err));
                        break;
                    }

                    // EOF?
                    if (bytes_read == 0)
                        break;

                    // Update buffer usage
                    int bufPos = 0;
                    bufUsed += bytes_read;

                    // Handle skipping
                    if (optSkip)
                    {
                        uint32_t skip = optSkip > bytes_read ? bytes_read : optSkip;
                        optSkip -= skip;
                        offset += skip;
                        bufUsed -= skip;
                        bufPos += skip;
                    }

                    if (bufUsed > optLength)
                    {
                        bufUsed = optLength;
                    }

                    // Output hex
                    while (bufUsed > 16)
                    {
                        hexdump(offset, buf + bufPos, 16);
                        bufPos += 16;
                        bufUsed -= 16;
                        offset += 16;
                        optLength -= 16;
                    }

                    // Move the left over back to start of buffer
                    if (bufUsed)
                    {
                        memcpy(buf, buf + bufPos, bufUsed);
                    }
                }

                f_close(&file);
            }
        }
    }

    if (bufUsed > optLength)
    {
        bufUsed = optLength;
    }

    if (bufUsed)
    {
        hexdump(offset, buf, bufUsed);
    }

    return end_enum_args(&args);
}

// Copy a file
int f_copyfile(const char* pszDest, const char* pszSrc, bool optOverwrite)
{
    // Check destination
    FILINFO fiDest;
    int err = f_stat(pszDest, &fiDest);
    if (err == 0)
    {
        // If overwrite disabled, ignore
        if (!optOverwrite)
            return 0;
        
        // If target is a directory then fail
        if (fiDest.fattrib & AM_DIR)
            return FR_EXIST;
    }

    // Open source file
    FIL src;
    err = f_open(&src, pszSrc, FA_OPEN_EXISTING | FA_READ);
    if (err)
        return err;

    // Open target file
    FIL dest;
    err = f_open(&dest, pszDest, FA_CREATE_ALWAYS | FA_WRITE);
    if (err)
    {
        f_close(&src);
        return err;
    }

    // Copy
    char buf[512];
    while (true)
    {
        // Read
        UINT bytes_read;
        err = f_read(&src, buf, sizeof(buf), &bytes_read);
        if (err)
            goto fail;

        // Write
        UINT bytes_written;
        err = f_write(&dest, buf, bytes_read, &bytes_written);
        if (err)
            goto fail;

        // EOF?
        if (bytes_read < sizeof(buf))
            break;
    }

    // Close files
    f_close(&src);
    f_close(&dest);

    // Get source stat
    FILINFO fi;
    err = f_stat(pszSrc, &fi);
    if (err)
        goto fail;

    // Update timestamp
    err = f_utime(pszDest, &fi);
    if (err)
        goto fail;

    // Update attributes
    err = f_chmod(pszDest, fi.fattrib, AM_ARC|AM_HID|AM_RDO|AM_SYS);
    if (err)
        goto fail;
    
    // Success!
    return 0;

fail:
    f_close(&src);
    f_close(&dest);
    f_unlink(pszDest);
    return err;
}

int f_copydir(const char* pszDest, const char* pszSrc, bool optOverwrite)
{
    // Make the target directory
    int err = f_mkdir(pszDest);
    if (err && err != FR_EXIST)
        return err;

    // Open the source directory
    DIR dir;
    err = f_opendir(&dir, pszSrc);
    if (err)
        return err;

    char szTarget[FF_MAX_LFN];
    char szSource[FF_MAX_LFN];

    bool errFlag = false;

    // Copy all items
    while (true)
    {
        // Get next entry
        FILINFO fi;
        err = f_readdir(&dir, &fi);
        if (err)
            goto fail;

        // End of dir?
        if (fi.fname[0] == '\0')
            break;

        // Work out target name
        strcpy(szTarget, pszDest);
        pathcat(szTarget, fi.fname);
        strcpy(szSource, pszSrc);
        pathcat(szSource, fi.fname);

        // Get the target stat
        FILINFO fiTarget;
        if (f_stat(szTarget, &fiTarget) == 0)
        {
            // If not overwriting, ignore source as target already exists
            if (!optOverwrite)
            {
                continue;
            }

            // Can't overwrite a target directory with source file
            if ((fiTarget.fattrib & AM_DIR) && (fi.fattrib & AM_DIR) == 0)
            {
                perr("cp: can't overwrite directory '%s' with file\n", szTarget);
                errFlag = true;
                continue;
            }
        }

        // Is it a file or directory
        if (fi.fattrib & AM_DIR)
        {
            // Recusively copy directory
            if (f_copydir(szTarget, szSource, optOverwrite) != 0)
                errFlag = true;
        }
        else
        {
            // Copy file
            if (f_copyfile(szTarget, szSource, optOverwrite) != 0)
                errFlag = true;
        }
    }

    // Handle non-fatal error
    if (err == 0 && errFlag)
        err = -1;

    // Clean up
fail:
    f_closedir(&dir);
    return err;
}

int cmd_cp(CMD_CONTEXT* pctx)
{
    bool optRecursive = false;
    bool optOverwrite = true;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pctx->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-r") == 0 || strcmp(opt.pszOpt, "--recursive") == 0)
        {
            if (opt.pszValue == NULL)
            {
                optRecursive = true;
                continue;
            }
            else
            {
                perr("cp: unexpected value '%s'\n", opt.pszValue);
                return -1;
            }
        }
        else if (strcmp(opt.pszOpt, "-n") == 0 || strcmp(opt.pszOpt, "--no-clobber") == 0)
        {
            if (opt.pszValue == NULL)
            {
                optOverwrite = false;
                continue;
            }
            else
            {
                perr("cp: unexpected value '%s'\n", opt.pszValue);
                return -1;
            }
        }
        else
        {
            perr("cp: unknown option: '%s'\n", opt.pszOpt);
            return -1;            
        }
    }

    // Split off target args
    ARGS argsTarget;
    if (!split_args(pctx->pargs, -1, &argsTarget))
    {
        perr("cp: no target specified\n");
        return -1;
    }

    // Enum target args
    char szTarget[FF_MAX_LFN];
    bool bTargetIsDir = false;
    bool bTargetExists = false;
    szTarget[0] = '\0';
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pctx->cwd, &argsTarget);
    while (next_arg(&args, &arg))
    {
        if (szTarget[0])
        {
            perr("cp: multiple targets specified\n");
            abort_enum_args(&args, -1);
            continue;
        }

        // If target looks like directory it must exist
        if (pathisdir(arg.pszAbsolute) && arg.pfi == NULL)
        {
            perr("cp: target directory '%s' doesn't exist\n", arg.pszRelative);
            abort_enum_args(&args, -1);
            continue;
        }

        // Store target
        bTargetExists = arg.pfi != NULL;
        bTargetIsDir = arg.pfi != NULL && (arg.pfi->fattrib & AM_DIR) != 0;
        strcpy(szTarget, arg.pszAbsolute);
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    // Remember length of target and check specified
    char* pszEndTarget = &szTarget[strlen(szTarget)];
    if (pszEndTarget == szTarget)
    {
        perr("cp: no target specified\n");
        return -1;
    }

    // Process args
    start_enum_args(&args, pctx->cwd, pctx->pargs);
    while (next_arg(&args, &arg))
    {
        if (arg.pfi == NULL)
        {
            perr("cp: no such file or directory: '%s'\n", arg.pszRelative);
        }
        else
        {
            if (arg.pfi->fattrib & AM_DIR)
            {
                if (!optRecursive)
                {
                    perr("cp: ignoring '%s' is a directory\n", arg.pszRelative);
                }
                else
                {
                    if (bTargetExists)
                    {
                        // If target exists and it's a directory then
                        // copy into the directory instead of "over" it
                        if (bTargetIsDir)
                        {
                            *pszEndTarget = '\0';
                            pathcat(szTarget, arg.pfi->fname);
                        }
                        else
                        {
                            if (optOverwrite)
                                f_unlink(szTarget);
                            else
                                continue;
                        }
                    }

                    // Check not trying to copy a directory to itself
                    if (pathcontains(arg.pszAbsolute, szTarget, false))
                    {
                        perr("cp: can't copy '%s' into itself\n", arg.pszRelative);
                        set_enum_args_error(&args, -1);
                        continue;
                    }

                    // Copy directory
                    int err = f_copydir(szTarget, arg.pszAbsolute, optOverwrite);
                    if (err)
                    {
                        perr("cp: failed to copy '%s' -> '%s', %s (%i)\n", arg.pszRelative, szTarget, f_strerror(err), err);
                        set_enum_args_error(&args, f_maperr(err));
                    }
                }
            }
            else
            {
                // Work out target path
                if (bTargetIsDir)
                {
                    *pszEndTarget = '\0';
                    pathcat(szTarget, arg.pfi->fname);
                }

                // Copy file
                int err = f_copyfile(szTarget, arg.pszAbsolute, optOverwrite);
                if (err)
                {
                    perr("cp: failed to copy '%s' -> '%s', %s (%i)", arg.pszRelative, szTarget, f_strerror(err), err);
                    set_enum_args_error(&args, f_maperr(err));
                }
            }
        }
    }

    return end_enum_args(&args);
}


// Invoke a command
int cmd(CMD_CONTEXT* pctx)
{
    if (pctx->pargs->argc < 1)
        return 0;

    // Save the command and remove it
    const char* pszCommand = pctx->pargs->argv[0];
    remove_arg(pctx->pargs, 0);

    if (strcmp(pszCommand, "touch") == 0)
        return cmd_touch(pctx);
    if (strcmp(pszCommand, "ls") == 0)
        return cmd_ls(pctx);
    if (strcmp(pszCommand, "rm") == 0)
        return cmd_rm(pctx);
    if (strcmp(pszCommand, "mkdir") == 0)
        return cmd_mkdir(pctx);
    if (strcmp(pszCommand, "cd") == 0)
        return cmd_cd(pctx);
    if (strcmp(pszCommand, "pwd") == 0)
        return cmd_pwd(pctx);
    if (strcmp(pszCommand, "rmdir") == 0)
        return cmd_rmdir(pctx);
    if (strcmp(pszCommand, "echo") == 0)
        return cmd_echo(pctx);
    if (strcmp(pszCommand, "cat") == 0)
        return cmd_cat(pctx);
    if (strcmp(pszCommand, "hexdump") == 0)
        return cmd_hexdump(pctx);
    if (strcmp(pszCommand, "cp") == 0)
        return cmd_cp(pctx);

    // cp
    // mv
    // pushd
    // popd

    perr("Unknown command '%s'\n", pszCommand);
    return 127;
}
