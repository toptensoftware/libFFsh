#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"

int cmd_cat(CMD_CONTEXT* pcmd)
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

    // Process args
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (arg.pfi == NULL)
        {
            perr("file not found: '%s'", arg.pszRelative);
        }
        else
        {
            if (arg.pfi->fattrib & AM_DIR)
            {
                perr("'%s' is a directory", arg.pszRelative);
            }
            else
            {
                // Open file
                FIL file;
                int err = f_open(&file, arg.pszAbsolute, FA_OPEN_EXISTING | FA_READ);
                if (err)
                {
                    perr("'%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
                    set_enum_args_error(&args, f_maperr(err));
                    continue;
                }

                // Read it
                char sz[129];
                while (true)
                {
                    // Read next buffer
                    UINT bytes_read;
                    err = f_read(&file, sz, sizeof(sz) - 1, &bytes_read);
                    if (err)
                    {
                        perr("'%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
                        set_enum_args_error(&args, f_maperr(err));
                        break;
                    }

                    // Print it
                    sz[bytes_read] = '\0';
                    pout("%s", sz);

                    if (bytes_read < sizeof(sz) -1)
                        break;
                }

                f_close(&file);
            }
        }
    }

    return end_enum_args(&args);
}

