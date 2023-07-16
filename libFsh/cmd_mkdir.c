#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"

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
