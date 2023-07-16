#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"

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
