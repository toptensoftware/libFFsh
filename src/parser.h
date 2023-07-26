#pragma once

#include "tokenizer.h"
#include "process.h"
#include "args.h"

enum NODETYPE
{
    NODETYPE_COMMAND,       // NODE_COMMAND
    NODETYPE_LOGICALAND,    // NODE_BINARY
    NODETYPE_LOGICALOR,     // NODE_BINARY
    NODETYPE_GROUP,         // NODE_BINARY
    NODETYPE_SUBSHELL,      // NODE_UNARY
    NODETYPE_NOP,           // NODE_NOP
};

struct NODE
{
    int type;
};

struct NODE_NOP
{
    int type;
};

struct NODE_UNARY
{
    int type;
    struct NODE* arg;
};

struct NODE_BINARY
{
    int type;
    struct NODE* lhs;
    struct NODE* rhs;
};

struct NODE_COMMAND
{
    int type;
    struct ARGS args;
};

struct NODE* parse(struct PROCESS* process, const char* psz);

