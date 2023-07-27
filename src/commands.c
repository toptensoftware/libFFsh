#include "process.h"
#include "commands.h"

START_COMMAND_TABLE(g_command_table)
    BUILTIN_COMMANDS
END_COMMAND_TABLE

bool dispatch_builtin_command(struct PROCESS* proc)
{
    return process_dispatch(proc, g_command_table);
}
