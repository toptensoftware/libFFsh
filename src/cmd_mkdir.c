#include "common.h"
#include "cmd.h"

int cmd_mkdir(struct PROCESS* proc)
{
    bool makeParents = false;

    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, proc, &proc->args);

    OPT opt;
    while (next_opt(&args, &opt))
    {
        if (is_switch(&args, &opt, "-p|--parents"))
            makeParents = true;
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
