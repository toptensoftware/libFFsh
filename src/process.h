#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "mempool.h"
#include "args.h"


void ffsh_printf(void (*write)(void*, char), void* arg, const char* format, ...);
void ffsh_sleep(uint32_t millis);
void ffsh_reboot();

struct PROCESS
{
    // Current working directory
    const char* cwd;
    
    // Memory pool for allocations that will be automatically
    // freed when the process ends
    struct MEMPOOL pool;

    // Arguments to the process
    const char* cmdname;
    struct ARGS args;

    // True if exit command invoked
    bool did_exit;

    // stdout
    void* user_stdout;
    void (*pfn_stdout)(void*,char);

    // stderr
    void* user_stderr;
    void (*pfn_stderr)(void*,char);

    // Long running commands will call this periodically
    void (*progress)();
};

#define printf_stderr(proc, fmt, ...) \
    ffsh_printf(proc->pfn_stderr, proc->user_stderr, fmt, ##__VA_ARGS__)

#define printf_stdout(proc, fmt, ...) \
    ffsh_printf(proc->pfn_stdout, proc->user_stdout, fmt, ##__VA_ARGS__)

#define perr(fmt, ...) \
    printf_stderr(proc, "%s: " fmt "\n", proc->cmdname, ##__VA_ARGS__)

#define pout(fmt, ...) \
    printf_stdout(proc, fmt, ##__VA_ARGS__)

// Initialize a new process
void process_init(struct PROCESS* process);

// Initialize a new process with same env as another
void process_dup(struct PROCESS* process, const struct PROCESS* from);

// Close a process
void process_close(struct PROCESS* process);

// Set CWD
void process_set_cwd(struct PROCESS* process, const char* cwd);

// Set STDIO callbacks
void process_set_stdout(struct PROCESS* process, void* user, void (*pfn)(void*,char));
void process_set_stderr(struct PROCESS* process, void* user, void (*pfn)(void*,char));

// Progress
void process_set_progress(struct PROCESS* process, void(*progress)());

// Parse and execute a shell command
int process_shell(struct PROCESS* proc, const char* psz);



void trace(const char* format, ...);

#define TRACE_POINT trace("==> %s %i\n", __FILE__, __LINE__);