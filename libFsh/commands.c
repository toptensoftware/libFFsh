#include "common.h"
#include <stdarg.h>
#include <errno.h>

#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"



int cmd_touch(CMD_CONTEXT* pcmd)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("touch: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    // Process args
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (!arg.pfi)
        {
            // Not if looks like a directory
            if (pathisdir(arg.pszRelative))
            {
                perr("no such directory '%s'", arg.pszRelative);
                set_enum_args_error(&args, ENOENT);
                continue;
            }

            // Create new file
            FIL f;
            int err = f_open(&f, arg.pszAbsolute, FA_CREATE_NEW | FA_WRITE);
            if (err)
            {
                perr("failed to create file '%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
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
                perr("failed to update times: '%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
                continue;
            }
        }
    }
    return end_enum_args(&args);
}

int cmd_ls_item(CMD_CONTEXT* pcmd, const char* pszRelative, FILINFO* pfi, bool optLong)
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

int cmd_ls_dir(CMD_CONTEXT* pcmd, const char* pszAbsolute, const char* pszRelative, bool optAll, bool optLong)
{
    DIR dir;
    int err = f_opendir(&dir, pszAbsolute);
    if (err)
    {
        perr("failed to opendir: '%s', %s (%i)", pszRelative, f_strerror(err), err);
        return f_maperr(err);
    }

    FILINFO fi;
    bool anyItems = false;
    while (true)
    {
        int err = f_readdir(&dir, &fi);
        if (err)
        {
            perr("failed to readdir: '%s', %s (%i)", pszRelative, f_strerror(err), err);
            f_closedir(&dir);
            return f_maperr(err);
        }
        if (fi.fname[0] == '\0')
            break;

        if (optAll || !f_is_hidden(&fi))
        {
            cmd_ls_item(pcmd, fi.fname, &fi, optLong);
            anyItems = true;
        }
    }

    if (anyItems && !optLong)
        printf("\n");

    f_closedir(&dir);
    return 0;
}

int cmd_ls(CMD_CONTEXT* pcmd)
{
    bool optAll = false;
    bool optLong = false;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-a") == 0)  
            optAll = true;
        else if (strcmp(opt.pszOpt, "-l") == 0)
            optLong = true;
        else
        {
            perr("unknown option: '%s'", opt.pszOpt);
            return -1;
        }
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
    bool anydirs = false;
    bool anyargs = false;
    bool anyitems = false;
    while (next_arg(&args, &arg))
    {
        anyargs = true;
        if (!arg.pfi)
        {
            perr("no such file or directory: '%s'", arg.pszRelative);
        }
        else
        {
            if ((arg.pfi->fattrib & AM_DIR) == 0)
            {
                int err = cmd_ls_item(pcmd, arg.pszRelative, arg.pfi, optLong);
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
        start_enum_args(&args, pcmd, pcmd->pargs);
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
                int err = cmd_ls_dir(pcmd, arg.pszAbsolute, arg.pszRelative, optAll, optLong);
                if (err)
                    set_enum_args_error(&args, err);
            }
        }
        result = end_enum_args(&args);        
    }

    if (!anyargs)
    {
        result = cmd_ls_dir(pcmd, pcmd->cwd, ".", optAll, optLong);
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



int cmd_rm(CMD_CONTEXT* pcmd)
{
    bool optRecursive = false;
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-r") == 0)
        {
            optRecursive = true;
            continue;
        }

        perr("unknown option: '%s'", opt.pszOpt);
        return -1;
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (!arg.pfi)
        {
            perr("no such file or directory: '%s'", arg.pszRelative);
        }
        else
        {
            if ((arg.pfi->fattrib & AM_DIR) == 0)
            {
                int err = f_unlink(arg.pszAbsolute);
                if (err)
                {
                    perr("removing '%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
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
                        perr("deleting '%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
                        set_enum_args_error(&args, f_maperr(err));;
                    }
                }
                else
                {
                    perr("cannot remove directory '%s'", arg.pszRelative);
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

int cmd_mkdir(CMD_CONTEXT* pcmd)
{
    bool makeParents = false;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
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
                perr("unexpected value '%s'", opt.pszValue);
                return -1;
            }
        }

        perr("unknown option: '%s'", opt.pszOpt);
        return -1;
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
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
                perr("'%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
            }
        }
        else
        {
            if (!makeParents)
                perr("already exists: '%s'", arg.pszRelative);
        }
    }
    return end_enum_args(&args);
}

int cmd_cd(CMD_CONTEXT* pcmd)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("unknown option: '%s'", opt.pszOpt);
        return -1;
    }

    char newcwd[FF_MAX_LFN];
    newcwd[0] = '\0';

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (!arg.pfi)
        {
            perr("path not found: '%s'", arg.pszRelative);
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
                    perr("too many arguments: '%s'", arg.pszRelative);
                    abort_enum_args(&args, -1);
                }
            }
        }
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    strcpy(pcmd->cwd, newcwd);
    return 0;
}

int cmd_pwd(CMD_CONTEXT* pcmd)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("unknown option: '%s'", opt.pszOpt);
        return -1;
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        perr("too many arguments: '%s'", arg.pszRelative);
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    pout("%s\n", pcmd->cwd);
    return 0;
}

int cmd_rmdir(CMD_CONTEXT* pcmd)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("unknown option: '%s'", opt.pszOpt);
        return -1;
    }

    // Process args (1st pass list files)
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        int err = f_rmdir(arg.pszAbsolute);
        if (err)
        {
            const char* pszErr = f_strerror(err);
            if (err == FR_DENIED)
                pszErr = "directory not empty";
            perr("removing '%s', %s (%i)", arg.pszRelative, pszErr, err);
            set_enum_args_error(&args, f_maperr(err));
        }
    }
    return end_enum_args(&args);
}

int cmd_echo(CMD_CONTEXT* pcmd)
{
    char szOutFile[FF_MAX_LFN];
    bool optOut = false;
    bool optAppend = false;
    bool optNoNewLine = false;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-o") == 0 || strcmp(opt.pszOpt, "--out") == 0)
        {
            if (optOut)
            {
                perr("multiple -o options");
                return -1;
            }
            if (opt.pszValue == NULL)
            {
                perr("-o argument missing");
                return -1;
            }
            if (pathisdir(opt.pszValue))
            {
                perr("'%s' is a directory", opt.pszValue);
            }
            strcpy(szOutFile, pcmd->cwd);
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
            perr("unknown option: '%s'", opt.pszOpt);
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
            perr("open file failed: '%s', %s (%i)", szOutFile, f_strerror(err), err);
            return err;
        }
    }

    // Process args
    ENUM_ARGS args;
    ARG arg;
    bool first = true;
    UINT unused;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (optOut)
        {
            if (!first)
                f_write(&file, " ", 1, &unused);
            int err = f_write(&file, arg.pszRelative, strlen(arg.pszRelative), &unused);
            if (err)
            {
                perr("failed to write: '%s', %s (%i)", szOutFile, f_strerror(err), err);
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

int cmd_cat(CMD_CONTEXT* pcmd)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("unknown option: '%s'", opt.pszOpt);
        return -1;
    }

    // Process args
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (arg.pfi == NULL)
        {
            perr("file not found: '%s'", arg.pszRelative);
        }
        else
        {
            if (arg.pfi->fattrib & AM_DIR)
            {
                perr("'%s' is a directory", arg.pszRelative);
            }
            else
            {
                // Open file
                FIL file;
                int err = f_open(&file, arg.pszAbsolute, FA_OPEN_EXISTING | FA_READ);
                if (err)
                {
                    perr("'%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
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
                        perr("'%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
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

void hexdump(CMD_CONTEXT* pcmd, uint32_t offset, const char* buf, int len)
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

int cmd_hexdump(CMD_CONTEXT* pcmd)
{
    uint32_t optSkip = 0;
    uint32_t optLength = 0xFFFFFFFF;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-s") == 0 || strcmp(opt.pszOpt, "--skip") == 0)
        {
            if (opt.pszValue == NULL)
            {
                perr("--skip argument missing");
                return -1;
            }
            else
            {   
                if (!parse_uint32(opt.pszValue, &optSkip))
                {
                    perr("invalid --skip argument '%s'", opt.pszValue);
                    return -1;
                }
            }
        }
        else if (strcmp(opt.pszOpt, "-n") == 0 || strcmp(opt.pszOpt, "--length") == 0)
        {
            if (opt.pszValue == NULL)
            {
                perr("--length argument missing");
                return -1;
            }
            else
            {   
                if (!parse_uint32(opt.pszValue, &optLength))
                {
                    perr("invalid --length argument '%s'", opt.pszValue);
                    return -1;
                }
            }
        }
        else
        {
            perr("unknown option: '%s'", opt.pszOpt);
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
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg) && offset < optSkip + optLength)
    {
        if (arg.pfi == NULL)
        {
            perr("file not found: '%s'", arg.pszRelative);
        }
        else
        {
            if (arg.pfi->fattrib & AM_DIR)
            {
                perr("'%s' is a directory", arg.pszRelative);
            }
            else
            {
                // Open file
                FIL file;
                int err = f_open(&file, arg.pszAbsolute, FA_OPEN_EXISTING | FA_READ);
                if (err)
                {
                    perr("'%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
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
                        perr("reading file '%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
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
                        hexdump(pcmd, offset, buf + bufPos, 16);
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
        hexdump(pcmd, offset, buf, bufUsed);
    }

    return end_enum_args(&args);
}


int f_copydir(CMD_CONTEXT* pcmd, const char* pszDest, const char* pszSrc, bool optOverwrite)
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
                perr("can't overwrite directory '%s' with file", szTarget);
                errFlag = true;
                continue;
            }
        }

        // Is it a file or directory
        if (fi.fattrib & AM_DIR)
        {
            // Recusively copy directory
            if (f_copydir(pcmd, szTarget, szSource, optOverwrite) != 0)
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

int cmd_cp(CMD_CONTEXT* pcmd)
{
    bool optRecursive = false;
    bool optOverwrite = true;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
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
                perr("unexpected value '%s'", opt.pszValue);
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
                perr("unexpected value '%s'", opt.pszValue);
                return -1;
            }
        }
        else
        {
            perr("unknown option: '%s'", opt.pszOpt);
            return -1;            
        }
    }

    // Split off target args
    ARGS argsTarget;
    if (!split_args(pcmd->pargs, -1, &argsTarget))
    {
        perr("no target specified");
        return -1;
    }

    // Enum target args
    char szTarget[FF_MAX_LFN];
    bool bTargetIsDir = false;
    bool bTargetExists = false;
    szTarget[0] = '\0';
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, &argsTarget);
    while (next_arg(&args, &arg))
    {
        if (szTarget[0])
        {
            perr("multiple targets specified");
            abort_enum_args(&args, -1);
            continue;
        }

        // Store target
        bTargetExists = arg.pfi != NULL;
        bTargetIsDir = pathisdir(arg.pszAbsolute) || (arg.pfi != NULL && (arg.pfi->fattrib & AM_DIR) != 0);
        strcpy(szTarget, arg.pszAbsolute);
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    // Remember length of target and check specified
    char* pszEndTarget = &szTarget[strlen(szTarget)];
    if (pszEndTarget == szTarget)
    {
        perr("no target specified");
        return -1;
    }

    // Process args
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (arg.pfi == NULL)
        {
            perr("no such file or directory: '%s'", arg.pszRelative);
        }
        else
        {
            if (arg.pfi->fattrib & AM_DIR)
            {
                if (!optRecursive)
                {
                    perr("ignoring '%s' is a directory", arg.pszRelative);
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
                        perr("can't copy '%s' into itself", arg.pszRelative);
                        set_enum_args_error(&args, -1);
                        continue;
                    }

                    // Copy directory
                    int err = f_copydir(pcmd, szTarget, arg.pszAbsolute, optOverwrite);
                    if (err)
                    {
                        perr("failed to copy '%s' -> '%s', %s (%i)", arg.pszRelative, szTarget, f_strerror(err), err);
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
                    perr("failed to copy '%s' -> '%s', %s (%i)", arg.pszRelative, szTarget, f_strerror(err), err);
                    set_enum_args_error(&args, f_maperr(err));
                }
            }
        }
    }

    return end_enum_args(&args);
}


int cmd_mv(CMD_CONTEXT* pcmd)
{
    bool optOverwrite = true;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-n") == 0 || strcmp(opt.pszOpt, "--no-clobber") == 0)
        {
            if (opt.pszValue == NULL)
            {
                optOverwrite = false;
                continue;
            }
            else
            {
                perr("unexpected value '%s'", opt.pszValue);
                return -1;
            }
        }
        else
        {
            perr("unknown option: '%s'", opt.pszOpt);
            return -1;            
        }
    }

    // Split off target args
    ARGS argsTarget;
    if (!split_args(pcmd->pargs, -1, &argsTarget))
    {
        perr("no target specified");
        return -1;
    }

    // Enum target args
    char szTarget[FF_MAX_LFN];
    bool bTargetIsDir = false;
    szTarget[0] = '\0';
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, &argsTarget);
    while (next_arg(&args, &arg))
    {
        if (szTarget[0])
        {
            perr("multiple targets specified");
            abort_enum_args(&args, -1);
            continue;
        }


        // Store target
        bTargetIsDir = pathisdir(arg.pszAbsolute) || (arg.pfi != NULL && (arg.pfi->fattrib & AM_DIR) != 0);
        strcpy(szTarget, arg.pszAbsolute);
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    // Remember length of target and check specified
    char* pszEndTarget = &szTarget[strlen(szTarget)];
    if (pszEndTarget == szTarget)
    {
        perr("no target specified");
        return -1;
    }

    // Process args
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (arg.pfi == NULL)
        {
            perr("no such file or directory: '%s'", arg.pszRelative);
        }
        else
        {
            // Work out target path
            if (bTargetIsDir)
            {
                *pszEndTarget = '\0';
                pathcat(szTarget, arg.pfi->fname);
            }

            // Check if it exists
            FILINFO fi;
            err = f_stat(szTarget, &fi);
            if (err)
            {
                if (err != FR_NO_FILE)
                {
                    perr(" failed to stat '%s', %s (%i)", szTarget, f_strerror(err), err);
                    set_enum_args_error(&args, f_maperr(err));
                    continue;
                }
            }
            else
            {
                if (optOverwrite)
                {
                    if (fi.fattrib & AM_DIR)
                    {
                        perr("can't overwrite '%s' as it's a directory", szTarget);
                        set_enum_args_error(&args, -1);
                        continue;
                    }

                    // Remove target
                    err = f_unlink(szTarget);
                    if (err)
                    {
                        perr("failed to unlink '%s', %s (%i)", szTarget, f_strerror(err), err);
                        set_enum_args_error(&args, f_maperr(err));
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }

            // Rename it
            int err = f_rename(arg.pszAbsolute, szTarget);
            if (err)
            {
                perr("failed to rename '%s' -> '%s', %s (%i)", arg.pszRelative, szTarget, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
            }
        }
    }

    return end_enum_args(&args);
}


// Invoke a command
int cmd(CMD_CONTEXT* pcmd)
{
    if (pcmd->pargs->argc < 1)
        return 0;

    // Save the command and remove it
    pcmd->cmdname = pcmd->pargs->argv[0];
    remove_arg(pcmd->pargs, 0);

    if (strcmp(pcmd->cmdname, "touch") == 0)
        return cmd_touch(pcmd);
    if (strcmp(pcmd->cmdname, "ls") == 0)
        return cmd_ls(pcmd);
    if (strcmp(pcmd->cmdname, "rm") == 0)
        return cmd_rm(pcmd);
    if (strcmp(pcmd->cmdname, "mkdir") == 0)
        return cmd_mkdir(pcmd);
    if (strcmp(pcmd->cmdname, "cd") == 0)
        return cmd_cd(pcmd);
    if (strcmp(pcmd->cmdname, "pwd") == 0)
        return cmd_pwd(pcmd);
    if (strcmp(pcmd->cmdname, "rmdir") == 0)
        return cmd_rmdir(pcmd);
    if (strcmp(pcmd->cmdname, "echo") == 0)
        return cmd_echo(pcmd);
    if (strcmp(pcmd->cmdname, "cat") == 0)
        return cmd_cat(pcmd);
    if (strcmp(pcmd->cmdname, "hexdump") == 0)
        return cmd_hexdump(pcmd);
    if (strcmp(pcmd->cmdname, "cp") == 0)
        return cmd_cp(pcmd);
    if (strcmp(pcmd->cmdname, "mv") == 0)
        return cmd_mv(pcmd);

    // pushd
    // popd

    perr("Unknown command");
    return 127;
}
