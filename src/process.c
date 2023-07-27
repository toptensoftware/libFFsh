#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "process.h"
#include "parser.h"
#include "commands.h"

void write_null(void* user, char ch)
{
    // nop
}

// Initialize a new process
void process_init(struct PROCESS* process)
{
    process->cwd = NULL;
    process->user_stderr = NULL;
    process->args.argc = 0;
    process->args.argv = NULL;
    process->cmdname = "shell";
    process->pfn_stderr = write_null;
    process->user_stdout = NULL;
    process->pfn_stdout = write_null;
    mempool_init(&process->pool, 4096);
    process_set_cwd(process, "/");
}

void process_dup(struct PROCESS* process, const struct PROCESS* from)
{
    process->cwd = NULL;
    process->args.argc = 0;
    process->args.argv = NULL;
    process->cmdname = "shell";
    process->user_stderr = from->user_stderr;
    process->pfn_stderr = from->pfn_stderr;
    process->user_stdout = from->user_stdout;
    process->pfn_stdout = from->pfn_stdout;
    mempool_init(&process->pool, 4096);
    process_set_cwd(process, from->cwd);
}


// Close a process
void process_close(struct PROCESS* process)
{
    mempool_release(&process->pool);
    free((char*)process->cwd);
}

// Set the process's cwd
void process_set_cwd(struct PROCESS* process, const char* cwd)
{
    // Allocate copy
    char* psz = (char*)malloc(strlen(cwd) + 1);
    strcpy(psz, cwd);

    // Release old
    if (process->cwd)
        free((char*)process->cwd);

    // Store new
    process->cwd = psz;
}

void process_set_stdout(struct PROCESS* process, void* user, void (*pfn)(void*,char))
{
    process->user_stdout = user;
    process->pfn_stdout = pfn;
}

void process_set_stderr(struct PROCESS* process, void* user, void (*pfn)(void*,char))
{
    process->user_stderr = user;
    process->pfn_stderr = pfn;
}

void process_set_progress(struct PROCESS* process, void(*progress)())
{
    process->progress = progress;
}

struct CMDINFO
{
    const char* name;
    int (*pfn_cmd)(struct PROCESS* proc);
};

#define COMMAND(name) { #name, cmd_ ## name }

struct CMDINFO g_cmdtable[] = {
    COMMAND(cat),
    COMMAND(cd),
    COMMAND(cp),
    COMMAND(echo),
    COMMAND(exit),
    COMMAND(false),
    COMMAND(hexdump),
    COMMAND(ls),
    COMMAND(mkdir),
    COMMAND(mv),
    COMMAND(pwd),
    COMMAND(rm),
    COMMAND(rmdir),
    COMMAND(sleep),
    COMMAND(touch),
    COMMAND(true),
};


// Dispatch a command
int process_invoke(struct PROCESS* proc, struct ARGS* pargs)
{
    proc->did_exit = false;

    // Nothing to do?
    if (pargs->argc < 0)
        return 0;

    // Remove the command name args
    proc->cmdname = pargs->argv[0];
    remove_arg(pargs, 0);

    // Store args
    proc->args = *pargs;

    // Dispatch command
    for (int i=0; i<sizeof(g_cmdtable) / sizeof(g_cmdtable[0]); i++)
    {
        struct CMDINFO* cmd = &g_cmdtable[i];
        if (strcmp(cmd->name, proc->cmdname) == 0)
        {
            return cmd->pfn_cmd(proc);
        }
    }

    // Unknown command
    printf_stderr(proc, "Unknown command\n");
    return 127;
}

static int process_eval_node(struct PROCESS* proc, struct NODE* node)
{
    switch (node->type)
    {
        case NODETYPE_COMMAND:
        {
            struct NODE_COMMAND* cmd = (struct NODE_COMMAND*)node;
            return process_invoke(proc, &cmd->args);
        }

        case NODETYPE_LOGICALAND:
        {
            struct NODE_BINARY* op = (struct NODE_BINARY*)node;
            int lhs = process_eval_node(proc, op->lhs);
            if (lhs != 0)
                return lhs;
            return process_eval_node(proc, op->rhs);
        }

        case NODETYPE_LOGICALOR:
        {
            struct NODE_BINARY* op = (struct NODE_BINARY*)node;
            int lhs = process_eval_node(proc, op->lhs);
            if (lhs == 0)
                return lhs;
            return process_eval_node(proc, op->rhs);
        }

        case NODETYPE_GROUP:
        {
            struct NODE_BINARY* op = (struct NODE_BINARY*)node;
            process_eval_node(proc, op->lhs);
            return process_eval_node(proc, op->rhs);
        }

        case NODETYPE_SUBSHELL:
        {
            struct NODE_UNARY* op = (struct NODE_UNARY*)node;
            struct PROCESS subp;
            process_dup(&subp, proc);
            int retv = process_eval_node(&subp, op->arg);
            process_close(&subp);
            return retv;
        }
    }
    assert(false);
    return 0;
}

int process_shell(struct PROCESS* proc, const char* psz)
{
    // Parse command line
    struct NODE* rootNode = parse(proc, psz);
    if (rootNode == NULL)
        return 127;

    // Process node
    return process_eval_node(proc, rootNode);
}