#include "common.h"
#include "cmd.h"

int cmd_ls_item(struct PROCESS* proc, const char* pszRelative, DIRENTRY* pfi, bool optLong)
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

int cmd_ls_dir(struct PROCESS* proc, const char* pszAbsolute, const char* pszRelative, bool optAll, bool optLong)
{
    DIREX dir;
    int err = f_opendir_ex(&dir, pszAbsolute, NULL, optAll ? NULL : direntry_filter_hidden, direntry_compare_name);
    if (err)
    {
        perr("failed to opendir: '%s', %s (%i)", pszRelative, f_strerror(err), err);
        return f_maperr(err);
    }

    DIRENTRY* fi;
    bool anyItems = false;
    while (f_readdir_ex(&dir, &fi))
    {
        cmd_ls_item(proc, fi->fname, fi, optLong);
        anyItems = true;
    }

    if (anyItems && !optLong)
        pout("\n");

    f_closedir_ex(&dir);
    return 0;
}

int cmd_ls(struct PROCESS* proc)
{
    bool optAll = false;
    bool optLong = false;

    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, proc, &proc->args);

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
                int err = cmd_ls_item(proc, arg.pszRelative, arg.pfi, optLong);
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
        pout("\n");
    int result = end_enum_args(&args);

    if (anydirs)
    {
        // Process args (2nd pass list directories)
        start_enum_args(&args, proc, &proc->args);
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
                int err = cmd_ls_dir(proc, arg.pszAbsolute, arg.pszRelative, optAll, optLong);
                if (err)
                    set_enum_args_error(&args, err);
            }
        }
        result = end_enum_args(&args);        
    }

    if (!anyargs)
    {
        result = cmd_ls_dir(proc, proc->cwd, ".", optAll, optLong);
    }

    return result;
}

