#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_args.h"
#include "ffex.h"

int cmd_echo(CMD_CONTEXT* pcmd)
{
    char szOutFile[FF_MAX_LFN];
    bool optOut = false;
    bool optAppend = false;
    bool optNoNewLine = false;

    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, pcmd, pcmd->pargs);
    
    OPT opt;
    while (next_opt(&args, &opt))
    {
        if (is_value_opt(&args, &opt, "-o|--out"))
        {
            if (pathisdir(opt.pszValue))
            {
                perr("'%s' is a directory", opt.pszValue);
                abort_enum_args(&args, -1);
            }
            strcpy(szOutFile, pcmd->cwd);
            pathcat(szOutFile, opt.pszValue);
            pathcan(szOutFile);
            optOut = true;
        }
        else if (is_switch(&args, &opt, "-a|--append"))
            optAppend = true;
        else if (is_switch(&args, &opt, "-n"))
            optNoNewLine = true;
        else
            unknown_opt(&args, &opt);
    }
    if (enum_args_error(&args))
        return end_enum_args(&args);

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
    ARG arg;
    bool first = true;
    UINT unused;
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
