#pragma once

struct PROCESS;
struct CMDINFO;

// The commands
int cmd_cat(struct PROCESS* proc);
int cmd_cd(struct PROCESS* proc);
int cmd_cp(struct PROCESS* proc);
int cmd_date(struct PROCESS* proc);
int cmd_echo(struct PROCESS* proc);
int cmd_exit(struct PROCESS* proc);
int cmd_false(struct PROCESS* proc);
int cmd_hexdump(struct PROCESS* proc);
int cmd_label(struct PROCESS* proc);
int cmd_ls(struct PROCESS* proc);
int cmd_mkdir(struct PROCESS* proc);
int cmd_mv(struct PROCESS* proc);
int cmd_pwd(struct PROCESS* proc);
int cmd_reboot(struct PROCESS* proc);
int cmd_rm(struct PROCESS* proc);
int cmd_rmdir(struct PROCESS* proc);
int cmd_sleep(struct PROCESS* proc);
int cmd_touch(struct PROCESS* proc);
int cmd_true(struct PROCESS* proc);


#define START_COMMAND_TABLE(name) \
    struct CMDINFO name[] = {

#define COMMAND(name) { #name, cmd_ ## name },

#define END_COMMAND_TABLE \
        { NULL, NULL } \
    };

#define BUILTIN_COMMANDS \
    COMMAND(cat) \
    COMMAND(cd) \
    COMMAND(cp) \
    COMMAND(date) \
    COMMAND(echo) \
    COMMAND(exit) \
    COMMAND(false) \
    COMMAND(hexdump) \
    COMMAND(label) \
    COMMAND(ls) \
    COMMAND(mkdir) \
    COMMAND(mv) \
    COMMAND(pwd) \
    COMMAND(reboot) \
    COMMAND(rm) \
    COMMAND(rmdir) \
    COMMAND(sleep) \
    COMMAND(touch) \
    COMMAND(true)

bool dispatch_builtin_command(struct PROCESS* proc);
