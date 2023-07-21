#pragma once

// Externally linked function you must provide
void ffsh_printf(void (*write)(void*, char), void* arg, const char* format, ...);
void ffsh_sleep(uint32_t millis);

// Helper to store argv/argc
typedef struct 
{
    const char** argv;
    int argc;
} ARGS;


// Command context
typedef struct
{
// PUBLIC: Client should setup these

    // Current working directory
    char* cwd;

    // stdout and stderr
    void* user;
    void (*pfn_stdout)(void*,char);
    void (*pfn_stderr)(void*,char);

// PRIVATE: libFFsh will manage these

    // Name of the current command
    const char* cmdname;
    bool did_exit;

    // Options and arguments
    ARGS* pargs;

} FFSH_CONTEXT;


// Parse a command and dispatch it
int ffsh_exec(FFSH_CONTEXT* pcmd, char* command);

// Dispatch an already parsed command
int ffsh_dispatch(FFSH_CONTEXT* pcmd);

// The commands
int cmd_cat(FFSH_CONTEXT* pcmd);
int cmd_cd(FFSH_CONTEXT* pcmd);
int cmd_cp(FFSH_CONTEXT* pcmd);
int cmd_echo(FFSH_CONTEXT* pcmd);
int cmd_exit(FFSH_CONTEXT* pcmd);
int cmd_false(FFSH_CONTEXT* pcmd);
int cmd_hexdump(FFSH_CONTEXT* pcmd);
int cmd_ls(FFSH_CONTEXT* pcmd);
int cmd_mkdir(FFSH_CONTEXT* pcmd);
int cmd_mv(FFSH_CONTEXT* pcmd);
int cmd_pwd(FFSH_CONTEXT* pcmd);
int cmd_rm(FFSH_CONTEXT* pcmd);
int cmd_rmdir(FFSH_CONTEXT* pcmd);
int cmd_sleep(FFSH_CONTEXT* pcmd);
int cmd_touch(FFSH_CONTEXT* pcmd);
int cmd_true(FFSH_CONTEXT* pcmd);

