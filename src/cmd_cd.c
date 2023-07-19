#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_args.h"
#include "ffex.h"

int cmd_cd(FFSH_CONTEXT* pcmd)
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

    char newcwd[FF_MAX_LFN];
    newcwd[0] = '\0';

    // Process args (1st pass list files)
    ARG arg;
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
    if (enum_args_error(&args))
        return end_enum_args(&args);

    strcpy(pcmd->cwd, newcwd);
    return 0;
}
