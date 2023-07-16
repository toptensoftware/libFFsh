#include "common.h"

#include "commands.h"
#include "path.h"
#include "args.h"
#include "enum_opts.h"
#include "enum_args.h"
#include "ffex.h"
#include "parse.h"

void hexdump(CMD_CONTEXT* pcmd, uint32_t offset, const char* buf, int len)
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

int cmd_hexdump(CMD_CONTEXT* pcmd)
{
    uint32_t optSkip = 0;
    uint32_t optLength = 0xFFFFFFFF;

    // Process options
    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, pcmd->pargs);
    while (next_opt(&opts, &opt))
    {
        if (strcmp(opt.pszOpt, "-s") == 0 || strcmp(opt.pszOpt, "--skip") == 0)
        {
            if (opt.pszValue == NULL)
            {
                perr("--skip argument missing");
                return -1;
            }
            else
            {   
                if (!parse_uint32(opt.pszValue, &optSkip))
                {
                    perr("invalid --skip argument '%s'", opt.pszValue);
                    return -1;
                }
            }
        }
        else if (strcmp(opt.pszOpt, "-n") == 0 || strcmp(opt.pszOpt, "--length") == 0)
        {
            if (opt.pszValue == NULL)
            {
                perr("--length argument missing");
                return -1;
            }
            else
            {   
                if (!parse_uint32(opt.pszValue, &optLength))
                {
                    perr("invalid --length argument '%s'", opt.pszValue);
                    return -1;
                }
            }
        }
        else
        {
            perr("unknown option: '%s'", opt.pszOpt);
            return -1;
        }
    }

    // Read buffer
    char buf[128];
    int bufUsed = 0;
    uint32_t offset = 0;

    // Process args
    ENUM_ARGS args;
    ARG arg;
    start_enum_args(&args, pcmd, pcmd->pargs);
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
                        hexdump(pcmd, offset, buf + bufPos, 16);
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
        hexdump(pcmd, offset, buf, bufUsed);
    }

    return end_enum_args(&args);
}

