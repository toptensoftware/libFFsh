#include "common.h"
#include "cmd.h"
#include <time.h>

int cmd_date(struct PROCESS* proc)
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
        return enum_args_error(&args);

    // Process args
    ARG arg;
    while (next_arg(&args, &arg))
    {
        perr("too many arguments: '%s'", arg.pszRelative);
        set_enum_args_error(&args, -1);
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    DWORD date = get_fattime();
    pout("%02i/%02i/%04i %02i:%02i:%02i\n", 
        ((date >> 16) & 0x1f),
        ((date >> 21) & 0x0f),
        ((date >> 25) & 0x7f) + 1980,
        ((date >> 11) & 0x1f),
        ((date >> 5) & 0x3f),
        ((date >> 0) & 0x1f) * 2
    );
    return 0;
}

