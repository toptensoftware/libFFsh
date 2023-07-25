#include "common.h"
#include "cmd.h"

void hexdump(struct PROCESS* proc, uint32_t offset, const char* buf, int len)
{
    pout("%08x: ", offset);

    for (int i=0; i<len; i++)
    {
        if (i == 8)
            pout(" ");
        pout("%02x ", buf[i]);
    }

    for (int i=len; i < 16; i++)
    {
        if (i == 8)
            pout(" ");
        pout("   ");
    }

    pout("| ");

    for (int i=0; i<len; i++)
    {
        if (buf[i] >= 32 && buf[i] <= 0x7f)
        {
            pout("%c", buf[i]);
        }
        else
        {
            pout(".");
        }
    }

    pout("\n");
}

int cmd_hexdump(struct PROCESS* proc)
{
    uint32_t optSkip = 0;
    uint32_t optLength = 0xFFFFFFFF;

    // Process options
    ENUM_ARGS args;
    start_enum_args(&args, proc, &proc->args);

    OPT opt;
    while (next_opt(&args, &opt))
    {
        if (is_value_opt(&args, &opt, "-s|--skip"))
        {
            if (!parse_uint32(opt.pszValue, &optSkip))
            {
                perr("invalid --skip argument '%s'", opt.pszValue);
                set_enum_args_error(&args, -1);
            }
        }
        else if (is_value_opt(&args, &opt, "-n|--length"))
        {
            if (!parse_uint32(opt.pszValue, &optLength))
            {
                perr("invalid --length argument '%s'", opt.pszValue);
                set_enum_args_error(&args, -1);
            }
        }
        else
            unknown_opt(&args, &opt);
    }
    if (enum_args_error(&args))
        return end_enum_args(&args);

    // Read buffer
    char buf[128];
    int bufUsed = 0;
    uint32_t offset = 0;

    // Process args
    ARG arg;
    while (next_arg(&args, &arg) && offset < optSkip + optLength)
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
                while (true)
                {
                    // Read next buffer
                    UINT bytes_read;
                    err = f_read(&file, buf + bufUsed, sizeof(buf) - bufUsed, &bytes_read);
                    if (err)
                    {
                        perr("reading file '%s', %s (%i)", arg.pszRelative, f_strerror(err), err);
                        set_enum_args_error(&args, f_maperr(err));
                        break;
                    }

                    // EOF?
                    if (bytes_read == 0)
                        break;

                    // Update buffer usage
                    int bufPos = 0;
                    bufUsed += bytes_read;

                    // Handle skipping
                    if (optSkip)
                    {
                        uint32_t skip = optSkip > bytes_read ? bytes_read : optSkip;
                        optSkip -= skip;
                        offset += skip;
                        bufUsed -= skip;
                        bufPos += skip;
                    }

                    if (bufUsed > optLength)
                    {
                        bufUsed = optLength;
                    }

                    // Output hex
                    while (bufUsed > 16)
                    {
                        hexdump(proc, offset, buf + bufPos, 16);
                        bufPos += 16;
                        bufUsed -= 16;
                        offset += 16;
                        optLength -= 16;
                    }

                    // Move the left over back to start of buffer
                    if (bufUsed)
                    {
                        memcpy(buf, buf + bufPos, bufUsed);
                    }
                }

                f_close(&file);
            }
        }
    }

    if (bufUsed > optLength)
    {
        bufUsed = optLength;
    }

    if (bufUsed)
    {
        hexdump(proc, offset, buf, bufUsed);
    }

    return end_enum_args(&args);
}

