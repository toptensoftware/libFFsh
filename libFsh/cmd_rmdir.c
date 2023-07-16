#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"

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

