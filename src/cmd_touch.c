#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_args.h"
#include "ffex.h"


int cmd_touch(FFSH_CONTEXT* pcmd)
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
    ARG arg;
    while (next_arg(&args, &arg))
    {
        if (!arg.pfi)
        {
            // Not if looks like a directory
            if (pathisdir(arg.pszRelative))
            {
                perr("no such directory '%s'", arg.pszRelative);
                set_enum_args_error(&args, ENOENT);
                continue;
            }

            // Create new file
            FIL f;
            int err = f_open(&f, arg.pszAbsolute, FA_CREATE_NEW | FA_WRITE);
            if (err)
            {
                perr("failed to create file '%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
                continue;
            }
            f_close(&f);
        }
        else
        {
            // Touch existing file/directory
            FILINFO fno;
            DWORD dwTime = get_fattime();
            fno.fdate = dwTime >> 16;
            fno.ftime = dwTime & 0xFFFF;
            int err = f_utime(arg.pszAbsolute, &fno);
            if (err)
            {
                perr("failed to update times: '%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
                set_enum_args_error(&args, f_maperr(err));
                continue;
            }
        }
    }
    return end_enum_args(&args);
}

