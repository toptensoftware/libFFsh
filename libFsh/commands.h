#pragma once

#include "args.h"

void fsh_printf(void (*write)(void*, char), void* arg, const char* format, ...);

#define perr(fmt, ...) \
    fsh_printf(pcmd->pfn_stderr, pcmd->user, "%s: " fmt "\n", pcmd->cmdname, ##__VA_ARGS__)

#define pout(fmt, ...) \
    fsh_printf(pcmd->pfn_stderr, pcmd->user, fmt, ##__VA_ARGS__)

// --- Command context ---

// Command context
typedef struct
{
    // Name of the current command
    const char* cmdname;

    // Options and arguments
    ARGS* pargs;

    // Current working directory
    char* cwd;

    // stdout and stderr
    void* user;
    void (*pfn_stdout)(void*,char);
    void (*pfn_stderr)(void*,char);

} CMD_CONTEXT;

int cmd(CMD_CONTEXT* pctx);