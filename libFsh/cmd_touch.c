#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"


int cmd_touch(CMD_CONTEXT* pcmd)
{
    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        perr("touch: unknown option: '%s'\n", opt.pszOpt);
        return -1;
    }

    // Process args
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
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

