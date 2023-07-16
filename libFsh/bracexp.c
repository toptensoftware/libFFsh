#include "common.h"
#include "bracexp.h"
#include "utf.h"
#include "args.h"

// Parse context
typedef struct 
{
    uint32_t cp;
    const char* pstart;
    const char* pcurr;
    const char* pnext;
    void* pOps;
    size_t cbOps;
    size_t cbOpsUsed;
    int depth;
    int totalDepth;
    uint32_t* special_chars;
} CONTEXT;

enum opcode
{
    opcode_text,   
    opcode_expr,    
    opcode_seq,
};

typedef struct
{
    const char* psz;
} HEADER;

typedef struct
{
    uint8_t  opcode;    // opcode::text
    uint16_t offset;    // Offset of text from start of string
    uint16_t length;    // Length of text
} OPTEXT;

typedef struct
{
    uint8_t  opcode;    // opcode_expr
    uint16_t oplen;     // Total length of this op
    uint16_t count;     // Number of direct child elements
    uint16_t perms;     // Total number of permutations
    // OPS[count]
} OPEXPR;

typedef struct
{
    uint8_t opcode;     // opcode_seq
    uint16_t oplen;     // Total length of this op
    uint16_t count;     // number of elements in sequence
    uint16_t perms;     // Total number of permutations
    // OPS[count]
} OPSEQ;

OPSEQ* parse_seq(CONTEXT* pctx);



// Read the next input character
uint32_t next_char(CONTEXT* pctx)
{
    if (pctx->cp == 0)
        return 0;
    pctx->pcurr = pctx->pnext;
    pctx->cp = utf8_decode(&pctx->pnext);
    return pctx->cp;
}

void* alloc(CONTEXT* pctx, size_t bytes)
{
    // Check room
    if (pctx->cbOpsUsed + bytes > pctx->cbOps)
        return NULL;
    
    // Take it
    void* pMem = (char*)(pctx->pOps) + pctx->cbOpsUsed;
    pctx->cbOpsUsed += bytes;
    return pMem;
}

OPEXPR* parse_expr(CONTEXT* pctx)
{
    // Track brace depth
    pctx->depth++;

    // Track max depth
    if (pctx->depth > pctx->totalDepth)
        pctx->totalDepth = pctx->depth;

    // Skip the opening brace
    next_char(pctx);

    // Remember where alloc occurs
    uint16_t cbOldUsed = pctx->cbOpsUsed;

    // Allocate expression
    OPEXPR* pExpr = alloc(pctx, sizeof(OPEXPR));
    if (pExpr == NULL)
        return NULL;

    // Setup expression
    pExpr->perms = 0;
    pExpr->count = 0;
    pExpr->oplen = 0;
    pExpr->opcode = opcode_expr;

    while (pctx->cp != '\0')
    {
        // Parse an inner sequence
        OPSEQ* pSeq = parse_seq(pctx);
        if (pSeq == NULL)
            return NULL;

        // Update count and perms
        pExpr->count++;
        pExpr->perms += pSeq->perms;

        // Next value?
        if (pctx->cp == pctx->special_chars[SPECIAL_CHAR_COMMA])
        {
            next_char(pctx);
            continue;
        }
        else
        {
            break;
        }
    }

    // Skip closing brace
    if (pctx->cp == pctx->special_chars[SPECIAL_CHAR_CLOSEBRACE])
        next_char(pctx);

    // Reduce depth
    pctx->depth--;

    // Update length of this op
    pExpr->oplen = pctx->cbOpsUsed - cbOldUsed;

    return pExpr;
}

OPSEQ* parse_seq(CONTEXT* pctx)
{
    // Remember where alloc occurs
    uint16_t cbOldUsed = pctx->cbOpsUsed;

    // Allocate sequence op
    OPSEQ* pSeq = alloc(pctx, sizeof(OPSEQ));
    if (pSeq == NULL)
        return NULL;

    // Setup expression
    pSeq->perms = 1;
    pSeq->count = 0;
    pSeq->oplen = 0;
    pSeq->opcode = opcode_seq;

    const char* pUnflushed = pctx->pcurr;

    while (pctx->cp)
    {
        // If inside braces, then stop parsing element
        // on comma or closing brace
        if (pctx->depth > 0)
        {
            if (pctx->cp == pctx->special_chars[SPECIAL_CHAR_COMMA] || pctx->cp == pctx->special_chars[SPECIAL_CHAR_CLOSEBRACE])
                break;
        }

        // New braced expression?
        if (pctx->cp == pctx->special_chars[SPECIAL_CHAR_OPENBRACE])
        {
            // Flush text
            if (pctx->pcurr > pUnflushed)
            {
                OPTEXT* pot = alloc(pctx, sizeof(OPTEXT));
                if (pot == NULL)
                    return NULL;
                pot->opcode = opcode_text;
                pot->offset = pUnflushed - pctx->pstart;
                pot->length = pctx->pcurr - pUnflushed;
                pUnflushed = pctx->pcurr;
                pSeq->count++;
            }

            OPEXPR* pExpr = parse_expr(pctx);
            if (pExpr == NULL)
                return NULL;

            pSeq->perms *= pExpr->perms;
            pSeq->count++;

            pUnflushed = pctx->pcurr;
        }
        else
        {
            next_char(pctx);
        }
    }

    // Flush trailing text
    if (pctx->pcurr > pUnflushed)
    {
        OPTEXT* pot = alloc(pctx, sizeof(OPTEXT));
        if (pot == NULL)
            return NULL;
        pot->opcode = opcode_text;
        pot->offset = pUnflushed - pctx->pstart;
        pot->length = pctx->pcurr - pUnflushed;
        pUnflushed = pctx->pcurr;
        pSeq->count++;
    }

    // Update length of this op
    pSeq->oplen = pctx->cbOpsUsed - cbOldUsed;

    return pSeq;
}


int bracexp_prepare(const char* psz, uint32_t* special_chars, void* pBuf, size_t cbBuf)
{
    // Setup context 
    CONTEXT ctx;
    ctx.cp = 1;
    ctx.pstart = psz;
    ctx.pnext = psz;
    ctx.pOps = pBuf;
    ctx.cbOps = cbBuf;
    ctx.cbOpsUsed = 0;
    ctx.depth = 0;
    ctx.totalDepth = 0;
    ctx.special_chars = special_chars;

    // Setup header
    HEADER* pHeader = (HEADER*)alloc(&ctx, sizeof(HEADER));
    if (pHeader == NULL)
        return -1;

    // Store the passed string
    pHeader->psz = psz;

    // Load first character
    next_char(&ctx);

    // Parse the root sequence
    OPSEQ * pSeq = parse_seq(&ctx);
    if (pSeq == NULL)
        return -1;

    // If no braces found, return 0 as the total permutation count
    if (ctx.totalDepth == 0)
        return 0;

    // Otherwise return the actual permutation count
    return pSeq->perms;
}




typedef struct
{
    const char* psz;
    char* pDest;
} EXPAND_CONTEXT;

void* next_op(void* pOp)
{
    switch (*(uint8_t*)pOp)
    {
        case opcode_text:
            return (char*)pOp + sizeof(OPTEXT);

        case opcode_expr:
            return (char*)pOp + ((OPEXPR*)pOp)->oplen;

        case opcode_seq:
            return (char*)pOp + ((OPSEQ*)pOp)->oplen;
    }
    return NULL;
}

int op_perms(void* pOp)
{
    switch (*(uint8_t*)pOp)
    {
        case opcode_text:
            return 1;

        case opcode_expr:
            return ((OPEXPR*)pOp)->perms;

        case opcode_seq:
            return ((OPSEQ*)pOp)->perms;
    }
    return 0;
}

void expand(EXPAND_CONTEXT* pctx, int perm, void* pOp);


void expand_text(EXPAND_CONTEXT* pctx, OPTEXT* pOpText)
{
    // Copy text to destination buffer and increment position
    memcpy(pctx->pDest, pctx->psz + pOpText->offset, pOpText->length);
    pctx->pDest += pOpText->length;
}

void expand_expr(EXPAND_CONTEXT* pctx, int perm, OPEXPR* pOp)
{
    // Work out permutation
    perm %= pOp->perms;

    // Get the first sub-op
    void* pSubOp = pOp+1;

    // Find the correct sub-op and expand it
    while (true)
    {
        int opPerms = op_perms(pSubOp);
        if (perm < opPerms)
        {
            expand(pctx, perm, pSubOp);
            return;
        }
        pSubOp = next_op(pSubOp);
        perm -= opPerms;
    }
}

int count_perms(void* pOp, int count)
{
    // Work out how many permutations to the right
    int perms = 1;
    for (int i=0; i<count; i++)
    {
        perms *= op_perms(pOp);
        pOp = next_op(pOp);
    }
    return perms;
}

void expand_seq(EXPAND_CONTEXT* pctx, int perm, OPSEQ* pOp)
{
    perm %= pOp->perms;

    // Render all sequence elements
    void* pSubOp = pOp+1;
    for (int i=0; i<pOp->count; i++)
    {
        void* pNextOp = next_op(pSubOp);;

        // Count perms to the right
        int rhsPerms = count_perms(pNextOp, pOp->count - i - 1);

        // Render this op
        expand(pctx, perm / rhsPerms, pSubOp);

        // Get next sub op
        pSubOp = pNextOp;
    }
}

void expand(EXPAND_CONTEXT* pctx, int perm, void* pOp)
{
    // Process all instructions
    switch (*(uint8_t*)pOp)
    {
        case opcode_text:
            expand_text(pctx, (OPTEXT*)pOp);
            break;

        case opcode_expr:
            expand_expr(pctx, perm, (OPEXPR*)pOp);
            break;

        case opcode_seq:
            expand_seq(pctx, perm, (OPSEQ*)pOp);
            break;
    }
}

void bracexp_expand(char* pDest, int perm, const void* pOps)
{
    // Retrieve the string pointer
    HEADER* pHeader = (HEADER*)pOps;

    // Setup context
    EXPAND_CONTEXT ctx;
    ctx.psz = pHeader->psz;
    ctx.pDest = pDest;

    // Expand
    expand(&ctx, perm, (void*)(pHeader + 1));

    // Terminate
    *ctx.pDest = '\0';
}