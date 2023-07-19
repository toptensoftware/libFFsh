#include "common.h"
#include "commands.h"
#include <alloca.h>

int ffsh_exec(FFSH_CONTEXT* pcmd, char* command)
{
    // Parse args
    int argCount = count_argv(command);
    ARGS args;
    args.argv = (const char**)alloca(argCount * sizeof(const char*));
    args.argc = 0;
    parse_argv(command, &args, argCount);
    pcmd->pargs = &args;

    // No args?
    if (pcmd->pargs->argc < 1)
        return 0;

    // Save the command and remove it
    pcmd->cmdname = pcmd->pargs->argv[0];
    remove_arg(pcmd->pargs, 0);

    // Setup command context
    return ffsh_dispatch(pcmd);
}
