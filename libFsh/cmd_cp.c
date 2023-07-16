#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"


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

