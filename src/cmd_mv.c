#include "common.h"
#include "cmd.h"

int cmd_mv(struct PROCESS* proc)
{
    bool optOverwrite = true;

    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, proc, &proc->args);

    OPT opt;
    while (next_opt(&args, &opt))
    {
        if (is_switch(&args, &opt, "-n|--no-clobber"))
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
    szTarget[0] = '\0';
    ARG arg;
    start_enum_args(&args, proc, &argsTarget);
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
    if (enum_args_error(&args))
        return end_enum_args(&args);

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
            // Work out target path
            if (bTargetIsDir)
            {
                *pszEndTarget = '\0';
                pathcat(szTarget, arg.pfi->fname);
            }

            // Check if it exists
            FILINFO fi;
            int err = f_stat(szTarget, &fi);
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
            err = f_rename(arg.pszAbsolute, szTarget);
            if (err)
            {
                perr("failed to rename '%s' -> '%s', %s (%i)", arg.pszRelative, szTarget, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
            }
        }
    }

    return end_enum_args(&args);
}


