#include "common.h"
#include "commands.h"

// Dispatch a command
int ffsh_dispatch(FFSH_CONTEXT* pcmd)
{
    pcmd->did_exit = false;
    
    if (strcmp(pcmd->cmdname, "touch") == 0)
        return cmd_touch(pcmd);
    if (strcmp(pcmd->cmdname, "ls") == 0)
        return cmd_ls(pcmd);
    if (strcmp(pcmd->cmdname, "rm") == 0)
        return cmd_rm(pcmd);
    if (strcmp(pcmd->cmdname, "mkdir") == 0)
        return cmd_mkdir(pcmd);
    if (strcmp(pcmd->cmdname, "cd") == 0)
        return cmd_cd(pcmd);
    if (strcmp(pcmd->cmdname, "pwd") == 0)
        return cmd_pwd(pcmd);
    if (strcmp(pcmd->cmdname, "rmdir") == 0)
        return cmd_rmdir(pcmd);
    if (strcmp(pcmd->cmdname, "echo") == 0)
        return cmd_echo(pcmd);
    if (strcmp(pcmd->cmdname, "cat") == 0)
        return cmd_cat(pcmd);
    if (strcmp(pcmd->cmdname, "hexdump") == 0)
        return cmd_hexdump(pcmd);
    if (strcmp(pcmd->cmdname, "cp") == 0)
        return cmd_cp(pcmd);
    if (strcmp(pcmd->cmdname, "mv") == 0)
        return cmd_mv(pcmd);
    if (strcmp(pcmd->cmdname, "exit") == 0)
        return cmd_exit(pcmd);

    // pushd
    // popd

    perr("Unknown command");
    return 127;
}

