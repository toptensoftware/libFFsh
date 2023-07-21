#include "common.h"

#include "commands.h"
#include "args.h"
#include "enum_args.h"
#include "parse.h"


int cmd_sleep(FFSH_CONTEXT* pcmd)
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

    // Process args
    uint32_t sleep_millis = 0;
    ARG arg;
    while (next_arg(&args, &arg))
    {
        uint32_t millis;
        if (parse_millis(arg.pszRelative, &millis))
        {
            sleep_millis += millis;
        }
        else
        {
            perr("Invalid time period '%s'", arg.pszRelative);
            abort_enum_args(&args, -1);
        }
    }
    if (enum_args_error(&args))
        end_enum_args(&args);

    ffsh_sleep(sleep_millis);

    return 0;
}

