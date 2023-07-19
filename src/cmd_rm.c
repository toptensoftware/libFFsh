#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_args.h"
#include "ffex.h"

int cmd_rm(FFSH_CONTEXT* pcmd)
{
    bool optRecursive = false;

    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, pcmd, pcmd->pargs);

    OPT opt;
    while (next_opt(&args, &opt))
    {
        if (is_switch(&args, &opt, "-r|--recursive"))
            optRecursive = true;
        else
            unknown_opt(&args, &opt);
    }
    if (enum_args_error(&args))
        return end_enum_args(&args);

    // Process args (1st pass list files)
    ARG arg;
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

