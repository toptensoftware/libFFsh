#pragma once

// Helper to store argv/argc
struct ARGS
{
    const char** argv;
    int argc;
};

// Remove a command line arg
void remove_arg(struct ARGS* pargs, int position);

// Split arguments
bool split_args(struct ARGS* pargs, int position, struct ARGS* pTailArgs);

