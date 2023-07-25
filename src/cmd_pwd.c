#include "common.h"
#include "cmd.h"

int cmd_pwd(struct PROCESS* proc)
{
    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, proc, &proc->args);

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
        perr("too many arguments: '%s'", arg.pszRelative);
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    pout("%s\n", proc->cwd);
    return 0;
}

