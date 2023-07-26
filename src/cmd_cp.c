#include "common.h"
#include "cmd.h"


int f_copydir(struct PROCESS* proc, const char* pszDest, const char* pszSrc, bool optOverwrite)
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
            if (f_copydir(proc, szTarget, szSource, optOverwrite) != 0)
                errFlag = true;

            // Progress
            if (proc->progress)
                proc->progress();
        }
        else
        {
            // Copy file
            if (f_copyfile(szTarget, szSource, optOverwrite, proc->progress) != 0)
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

int cmd_cp(struct PROCESS* proc)
{
    bool optRecursive = false;
    bool optOverwrite = true;

    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, proc, &proc->args);
    
    OPT opt;
    while (next_opt(&args, &opt))
    {
        if (is_switch(&args, &opt, "-r|--recursive"))
            optRecursive = true;
        else if (is_switch(&args, &opt,"-n|--no-clobber"))
            optOverwrite = false;
        else
            unknown_opt(&args, &opt);
    }
    if (enum_args_error(&args))
        return end_enum_args(&args);

    // Split off target args
    struct ARGS argsTarget;
    if (!split_args(&proc->args, -1, &argsTarget))
    {
        perr("no target specified");
        return -1;
    }

    // Enum target args
    char szTarget[FF_MAX_LFN];
    bool bTargetIsDir = false;
    bool bTargetExists = false;
    szTarget[0] = '\0';
    ENUM_ARGS enum_targets;
    ARG arg;
    start_enum_args(&enum_targets, proc, &argsTarget);
    while (next_arg(&enum_targets, &arg))
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
    if (enum_args_error(&enum_targets))
        return end_enum_args(&enum_targets);

    // Remember length of target and check specified
    char* pszEndTarget = &szTarget[strlen(szTarget)];
    if (pszEndTarget == szTarget)
    {
        perr("no target specified");
        return -1;
    }

    // Process args
    start_enum_args(&args, proc, &proc->args);
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
                    int err = f_copydir(proc, szTarget, arg.pszAbsolute, optOverwrite);
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
                int err = f_copyfile(szTarget, arg.pszAbsolute, optOverwrite, proc->progress);
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

