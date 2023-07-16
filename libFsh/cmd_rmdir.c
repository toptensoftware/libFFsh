#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_args.h"
#include "ffex.h"

int cmd_rmdir(CMD_CONTEXT* pcmd)
{
    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, pcmd, pcmd->pargs);

    OPT opt;
    while (next_opt(&args, &opt))
    {
        unknown_opt(&args, &opt);
    }
    if (enum_args_error(&args))
        return end_enum_args(&args);

    // Process args (1st pass list files)
    ARG arg;
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

