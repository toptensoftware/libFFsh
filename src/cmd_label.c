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
    char szVolume[FF_MAX_LFN];
    char szArgs[FF_MAX_LFN];
    while (next_arg(&args, &arg))
    {
        if (!haveDrive)
        {
            if (arg.pfi == NULL || (arg.pfi->fattrib & AM_DRIVE)==0)
            {
                perr("not a valid volume '%s'", arg.pszRelative);
                abort_enum_args(&args, -1);
                continue;
            }

            strcpy(szVolume, arg.pszAbsolute);
            int err = f_realpath(szVolume);
            if (err)
            {
                perr("error resolving realpath '%s', %s (%i)", szVolume, f_strerror(err), err);
                abort_enum_args(&args, err);
                continue;
            }
            szArgs[0] = arg.pfi->fsize + '0';   // Logical drive number in fsize
            szArgs[1] = ':';
            szArgs[2] = '\0';
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
        int err = f_getlabel(szVolume, szLabel, &serial);
        if (err)
        {
            perr("error getting label for '%s' %s (%i)", szArgs, f_strerror(err), err);
            return err;
        }

        pout("%s %04X-%04X %s\n", szVolume, serial >> 16, serial & 0xFFFF, szLabel);
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

