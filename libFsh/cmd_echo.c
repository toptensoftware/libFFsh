#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"

int cmd_echo(CMD_CONTEXT* pcmd)
{
    char szOutFile[FF_MAX_LFN];
    bool optOut = false;
    bool optAppend = false;
    bool optNoNewLine = false;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-o") == 0 || strcmp(opt.pszOpt, "--out") == 0)
        {
            if (optOut)
            {
                perr("multiple -o options");
                return -1;
            }
            if (opt.pszValue == NULL)
            {
                perr("-o argument missing");
                return -1;
            }
            if (pathisdir(opt.pszValue))
            {
                perr("'%s' is a directory", opt.pszValue);
            }
            strcpy(szOutFile, pcmd->cwd);
            pathcat(szOutFile, opt.pszValue);
            pathcan(szOutFile);
            optOut = true;
        }
        else if (strcmp(opt.pszOpt, "-a") == 0 || strcmp(opt.pszOpt, "--append") == 0)
        {
            optAppend = true;
        }
        else if (strcmp(opt.pszOpt, "-n") == 0)
        {
            optNoNewLine = true;
        }
        else
        {
            perr("unknown option: '%s'", opt.pszOpt);
            return -1;
        }
    }

    // Open output file
    FIL file;
    if (optOut)
    {
        int err = f_open(&file, szOutFile, optAppend ? (FA_CREATE_NEW | FA_WRITE) : (FA_OPEN_ALWAYS | FA_OPEN_APPEND | FA_WRITE));
        if (err)
        {
            perr("open file failed: '%s', %s (%i)", szOutFile, f_strerror(err), err);
            return err;
        }
    }

    // Process args
    ENUM_ARGS args;
    ARG arg;
    bool first = true;
    UINT unused;
    start_enum_args(&args, pcmd, pcmd->pargs);
    while (next_arg(&args, &arg))
    {
        if (optOut)
        {
            if (!first)
                f_write(&file, " ", 1, &unused);
            int err = f_write(&file, arg.pszRelative, strlen(arg.pszRelative), &unused);
            if (err)
            {
                perr("failed to write: '%s', %s (%i)", szOutFile, f_strerror(err), err);
                f_close(&file);
                return f_maperr(err);
            }
        }
        else
        {
            if (!first)
                pout(" ");
            pout("%s", arg.pszRelative);
        }
        first = false;
    }

    if (!optNoNewLine)
    {
        if (optOut)
        {
            f_write(&file, "\n", 1, &unused);
        }
        else
        {
            pout("\n");
        }
    }


    if (optOut)
    {
        f_close(&file);
    }

    return end_enum_args(&args);
}
