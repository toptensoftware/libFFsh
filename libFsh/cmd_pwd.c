#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"

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

