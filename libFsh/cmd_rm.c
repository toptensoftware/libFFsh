#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"

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

