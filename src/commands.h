#pragma once

struct PROCESS;

// The commands
int cmd_cat(struct PROCESS* proc);
int cmd_cd(struct PROCESS* proc);
int cmd_cp(struct PROCESS* proc);
int cmd_echo(struct PROCESS* proc);
int cmd_exit(struct PROCESS* proc);
int cmd_false(struct PROCESS* proc);
int cmd_hexdump(struct PROCESS* proc);
int cmd_ls(struct PROCESS* proc);
int cmd_mkdir(struct PROCESS* proc);
int cmd_mv(struct PROCESS* proc);
int cmd_pwd(struct PROCESS* proc);
int cmd_rm(struct PROCESS* proc);
int cmd_rmdir(struct PROCESS* proc);
int cmd_sleep(struct PROCESS* proc);
int cmd_touch(struct PROCESS* proc);
int cmd_true(struct PROCESS* proc);

