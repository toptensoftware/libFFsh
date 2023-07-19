#pragma once

#include "args.h"

void fsh_printf(void (*write)(void*, char), void* arg, const char* format, ...);

#define perr(fmt, ...) \
    fsh_printf(pcmd->pfn_stderr, pcmd->user, "%s: " fmt "\n", pcmd->cmdname, ##__VA_ARGS__)

#define pout(fmt, ...) \
    fsh_printf(pcmd->pfn_stdout, pcmd->user, fmt, ##__VA_ARGS__)


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


int cmd_cat(CMD_CONTEXT* pcmd);
int cmd_cd(CMD_CONTEXT* pcmd);
int cmd_cp(CMD_CONTEXT* pcmd);
int cmd_echo(CMD_CONTEXT* pcmd);
int cmd_hexdump(CMD_CONTEXT* pcmd);
int cmd_ls(CMD_CONTEXT* pcmd);
int cmd_mkdir(CMD_CONTEXT* pcmd);
int cmd_mv(CMD_CONTEXT* pcmd);
int cmd_pwd(CMD_CONTEXT* pcmd);
int cmd_rm(CMD_CONTEXT* pcmd);
int cmd_rmdir(CMD_CONTEXT* pcmd);
int cmd_touch(CMD_CONTEXT* pcmd);



int cmd(CMD_CONTEXT* pctx);