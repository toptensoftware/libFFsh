#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_args.h"
#include "ffex.h"

int cmd_ls_item(FFSH_CONTEXT* pcmd, const char* pszRelative, FILINFO* pfi, bool optLong)
{
//    if (pszRelative[0] == '.' && (pszRelative[1] == '\\' || pszRelative[1] == '/'))
//        pszRelative += 2;
    if (optLong)
    {
        pout("%c%c%c%c%c %02i/%02i/%04i %02i:%02i:%02i %12i %s\n", 
            (pfi->fattrib & AM_DIR) ? 'd' : '-',
            (pfi->fattrib & AM_RDO) ? 'r' : '-',
            (pfi->fattrib & AM_SYS) ? 's' : '-',
            (pfi->fattrib & AM_HID) ? 'h' : '-',
            (pfi->fattrib & AM_ARC) ? 'a' : '-',
            (pfi->fdate & 0x1f),
            ((pfi->fdate >> 5) & 0x0f),
            ((pfi->fdate >> 9) & 0x7f) + 1980,
            ((pfi->ftime >> 11) & 0x1f),
            ((pfi->ftime >> 5) & 0x3f),
            ((pfi->ftime >> 0) & 0x1f) * 2,
            pfi->fsize,
            pszRelative);
    }
    else
    {
        pout("%s  ", pszRelative);
    }

    return 0;
}

int cmd_ls_dir(FFSH_CONTEXT* pcmd, const char* pszAbsolute, const char* pszRelative, bool optAll, bool optLong)
{
    DIR dir;
    int err = f_opendir(&dir, pszAbsolute);
    if (err)
    {
        perr("failed to opendir: '%s', %s (%i)", pszRelative, f_strerror(err), err);
        return f_maperr(err);
    }

    FILINFO fi;
    bool anyItems = false;
    while (true)
    {
        int err = f_readdir(&dir, &fi);
        if (err)
        {
            perr("failed to readdir: '%s', %s (%i)", pszRelative, f_strerror(err), err);
            f_closedir(&dir);
            return f_maperr(err);
        }
        if (fi.fname[0] == '\0')
            break;

        if (optAll || !f_is_hidden(&fi))
        {
            cmd_ls_item(pcmd, fi.fname, &fi, optLong);
            anyItems = true;
        }
    }

    if (anyItems && !optLong)
        printf("\n");

    f_closedir(&dir);
    return 0;
}

int cmd_ls(FFSH_CONTEXT* pcmd)
{
    bool optAll = false;
    bool optLong = false;

    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, pcmd, pcmd->pargs);

    OPT opt;
    while (next_opt(&args, &opt))
    {
        if (is_switch(&args, &opt, "-a|--all"))
            optAll = true;
        else if (is_switch(&args, &opt, "-l"))
            optLong = true;
        else
            unknown_opt(&args, &opt);
    }
    if (enum_args_error(&args))
        return end_enum_args(&args);

    // Process args (1st pass list files)
    ARG arg;
    bool anydirs = false;
    bool anyargs = false;
    bool anyitems = false;
    while (next_arg(&args, &arg))
    {
        anyargs = true;
        if (!arg.pfi)
        {
            perr("no such file or directory: '%s'", arg.pszRelative);
        }
        else
        {
            if ((arg.pfi->fattrib & AM_DIR) == 0)
            {
                int err = cmd_ls_item(pcmd, arg.pszRelative, arg.pfi, optLong);
                if (err)
                    set_enum_args_error(&args, err);
                anyitems = true;
            }
            else
            {
                anydirs = true;
            }
        }
    }
    if (anyitems && !optLong)
        printf("\n");
    int result = end_enum_args(&args);

    if (anydirs)
    {
        // Process args (2nd pass list directories)
        start_enum_args(&args, pcmd, pcmd->pargs);
        set_enum_args_error(&args, result);
        bool first = !anyitems;
        while (next_arg(&args, &arg))
        {
            if (arg.pfi && arg.pfi->fattrib & AM_DIR)
            {
                if (!first)
                    pout("\n");
                else
                    first = false;
                pout("%s:\n", arg.pszRelative);
                int err = cmd_ls_dir(pcmd, arg.pszAbsolute, arg.pszRelative, optAll, optLong);
                if (err)
                    set_enum_args_error(&args, err);
            }
        }
        result = end_enum_args(&args);        
    }

    if (!anyargs)
    {
        result = cmd_ls_dir(pcmd, pcmd->cwd, ".", optAll, optLong);
    }

    return result;
}

