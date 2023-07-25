#include "parser.h"

struct PARSER
{
    struct PROCESS* process;
    struct MEMPOOL* pool;
    struct TOKENIZER tokenizer;
    struct MEMSTREAM stream;
};

struct NODE* parseExpression(struct PARSER* parser);

struct NODE* parseLeaf(struct PARSER* parser)
{
    if (parser->tokenizer.token == TOKEN_ARG)
    {
        // Reset stream
        memstream_reset(&parser->stream);

        // Allocate node
        struct NODE_COMMAND* cmd = (struct NODE_COMMAND*)mempool_alloc(parser->pool, sizeof(struct NODE_COMMAND));
        cmd->type = NODETYPE_COMMAND;
        cmd->args.argc = 0;

        // Collect all consecutive args together
        while (parser->tokenizer.token == TOKEN_ARG)
        {
            memstream_write(&parser->stream, &parser->tokenizer.arg, sizeof(void*));
            tokenizer_next(&parser->tokenizer);
            cmd->args.argc++;
        }

        // Allocate argv array
        cmd->args.argv = mempool_alloc_copy(parser->pool, parser->stream.p, parser->stream.length);

        // Return node
        return (struct NODE*)cmd;
    }

    if (parser->tokenizer.token == TOKEN_OPENROUND)
    {
        tokenizer_next(&parser->tokenizer);

        struct NODE_UNARY* sub = (struct NODE_UNARY*)mempool_alloc(parser->pool, sizeof(struct NODE_UNARY));
        sub->type = NODETYPE_SUBSHELL;
        sub->arg = parseExpression(parser);
        if (sub->arg == NULL)
            return NULL;

        if (parser->tokenizer.token != TOKEN_CLOSEROUND)
        {
            printf_stderr(parser->process, "syntax error: missing ')'\n");
            return NULL;
        }

        tokenizer_next(&parser->tokenizer);

        return (struct NODE*)sub;
    }

    printf_stderr(parser->process, "syntax error: expected comamnd\n");
    return NULL;
}

struct NODE* parseBinary(struct PARSER* parser)
{
    // Parse left hand side
    struct NODE* lhs = parseLeaf(parser);
    if (!lhs)
        return NULL;

    while (parser->tokenizer.token == TOKEN_LOGICAL_AND ||
            parser->tokenizer.token == TOKEN_LOGICAL_OR ||
            parser->tokenizer.token == TOKEN_SEMICOLON)
    {
        // Save token, skip it
        int token = parser->tokenizer.token;
        tokenizer_next(&parser->tokenizer);

        // Parse RHS
        struct NODE* rhs = parseLeaf(parser);
        if (rhs == NULL)
            return NULL;

        // Allocate node
        struct NODE_BINARY* bin = (struct NODE_BINARY*)mempool_alloc(parser->pool, sizeof(struct NODE_BINARY));
        bin->lhs = lhs;
        bin->rhs = rhs;

        switch (token)
        {
            case TOKEN_LOGICAL_AND:
                bin->type = NODETYPE_LOGICALAND;
                break;

            case TOKEN_LOGICAL_OR:
                bin->type = NODETYPE_LOGICALOR;
                break;

            case TOKEN_SEMICOLON:
                bin->type = NODETYPE_GROUP;
                break;
        }

        lhs = (struct NODE*)bin;
    }

    return lhs;
}


// Parse a group of ';' delimited commands
struct NODE* parseGroup(struct PARSER* parser)
{
    // Parse left hand side
    struct NODE* lhs = parseBinary(parser);
    if (!lhs)
        return NULL;

    while (parser->tokenizer.token == TOKEN_SEMICOLON)
    {
        // Skip token
        tokenizer_next(&parser->tokenizer);

        // Parse RSH
        struct NODE* rhs = parseBinary(parser);
        if (rhs == NULL)
            return NULL;

        // Allocate node
        struct NODE_BINARY* bin = (struct NODE_BINARY*)mempool_alloc(parser->pool, sizeof(struct NODE_BINARY));
        bin->lhs = lhs;
        bin->rhs = rhs;
        bin->type = NODETYPE_GROUP;

        lhs = (struct NODE*)bin;
    }

    return lhs;
}

struct NODE* parseExpression(struct PARSER* parser)
{
    return parseBinary(parser);
}

struct NODE* parse(struct PROCESS* process, const char* psz)
{
    // Setup
    struct PARSER parser;
    parser.pool = &process->pool;
    parser.process = process;
    tokenizer_init(&parser.tokenizer, &process->pool, psz);
    memstream_initnew(&parser.stream, sizeof(void*) * 32);

    // Parse the root node
    struct NODE* rootNode = parseExpression(&parser);
    
    // Clean up
    memstream_close(&parser.stream);
    tokenizer_close(&parser.tokenizer);
    return rootNode;
}