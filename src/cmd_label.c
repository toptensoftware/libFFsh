#include "common.h"
#include "cmd.h"

int cmd_label(struct PROCESS* proc)
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
    bool haveDrive = false;
    bool haveNewLabel = false;
    char szArgs[FF_MAX_LFN];
    while (next_arg(&args, &arg))
    {
        if (!haveDrive)
        {
            const char* pathPart = pathskipdrive(arg.pszRelative);
            if (pathPart == NULL || pathPart[0] != '\0')
            {
                perr("not a valid drive specifier '%s'", arg.pszRelative);
                set_enum_args_error(&args, -1);
                continue;
            }

            strcpy(szArgs, arg.pszRelative);
            haveDrive = true;
        }
        else if (!haveNewLabel)
        {
            strcat(szArgs, arg.pszRelative);
            haveNewLabel = true;
        }
        else
        {
            perr("too many arguments: '%s'", arg.pszRelative);
            set_enum_args_error(&args, -1);
        }
    }
    int err = end_enum_args(&args);
    if (err)
        return err;

    if (!haveDrive)
    {
        perr("missing drive specifier");
        return -1;
    }

    if (!haveNewLabel)
    {
        DWORD serial;
        char szLabel[FF_MAX_LFN];
        int err = f_getlabel(szArgs, szLabel, &serial);
        if (err)
        {
            perr("error getting label for '%s' %s (%i)", arg.pszRelative, f_strerror(err), err);
            return err;
        }

        pout("%s %04X-%04X %s\n", arg.pszRelative, serial >> 16, serial & 0xFFFF, szLabel);
    }
    else
    {
        int err = f_setlabel(szArgs);
        if (err)
        {
            perr("error setting label '%s' %s (%i)", szArgs, f_strerror(err), err);
            return err;
        }
    }

    return 0;
}

