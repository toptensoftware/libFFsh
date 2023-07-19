#pragma once

#include "args.h"
#include "ffsh.h"

#define perr(fmt, ...) \
    ffsh_printf(pcmd->pfn_stderr, pcmd->user, "%s: " fmt "\n", pcmd->cmdname, ##__VA_ARGS__)

#define pout(fmt, ...) \
    ffsh_printf(pcmd->pfn_stdout, pcmd->user, fmt, ##__VA_ARGS__)

