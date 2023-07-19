#include <stdio.h>
#include <alloca.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include "diskio.h"
#include "../ff15/source/ff.h"
#include "../src/commands.h"
#include "../src/path.h"
#include "../src/bracexp.h"

void fsh_printf(void (*write)(void*, char), void* arg, const char* format, ...)
{
    char szTemp[FF_MAX_LFN + 100];

	va_list args;
	va_start(args, format);
    vsnprintf(szTemp, sizeof(szTemp), format, args);
    va_end(args);

    const char* p = szTemp;
    while (*p)
    {
        write(arg, *p++);
    }
}

void pfn_stderr(void*, char ch)
{
    fputc(ch, stderr);
}

void pfn_stdout(void*, char ch)
{
    fputc(ch, stdout);
}


int main()
{
    /*
    char cl[] = "command -a --sw --arg:XXX -abc -xyz:YYY";
    int count = count_argv(cl);
    ARGS args;
    args.argv = (const char**)alloca(sizeof(char*) * count);
    parse_argv(cl, &args, count);
    for (int i=0; i<args.argc; i++)
    {
        printf("%i: %s\n", i, args.argv[i]);
    }

    ENUM_OPTS opts;
    OPT opt;
    enum_opts(&opts, &args);
    while (next_opt(&opts, &opt))
    {
        printf("%s %s\n", opt.pszOpt, opt.pszValue ? opt.pszValue : "null");
    }
    return 0;
    printf("%i\n", is_opt("-a", "-a|--longx"));
    return 0;
    */

    // Initialize disk
    printf("Mounting disk... ");
    int err = disk_mount();
    if (err)
    {
        printf("failed (%i)\n", err);
        printf("Running f_mkfs...");
        MKFS_PARM mkfsp;
        memset(&mkfsp, 0, sizeof(mkfsp));
        mkfsp.fmt = FM_ANY;
        mkfsp.n_fat = 2;
        char work[512];
        err = f_mkfs("0:", &mkfsp, work, sizeof(work));
        if (err)
        {
            printf("failed %i\n", err);
            return err;
        }
        printf("ok\n");
        printf("Mounting disk... ");
        err = disk_mount();
    }
    if (err)
    {
        printf("failed %i\n", err);
        return err;
    }
    printf("ok\n");

    char cwd[FF_MAX_LFN];
    strcpy(cwd, "/");

    while (true)
    {
        // Prompt
        printf("%s>", cwd);

        // Get next command
        char szCommand[512];
        if (!fgets(szCommand, sizeof(szCommand), stdin))
            break;
        char* psz = szCommand;
        while (*psz != '\n' && *psz != '\0')
            psz++;
        *psz = '\0';

        // Parse args
        int argCount = count_argv(szCommand);
        ARGS args;
        args.argv = (const char**)alloca(argCount * sizeof(const char*));
        args.argc = 0;
        parse_argv(szCommand, &args, argCount);

        // Exit?
        if (args.argc > 0 && strcmp(args.argv[0], "exit") == 0)
            break;

        // Setup command context
        CMD_CONTEXT ctx;
        ctx.pargs = &args;
        ctx.cwd = cwd;
        ctx.user = NULL;
        ctx.pfn_stderr = pfn_stderr;
        ctx.pfn_stdout = pfn_stdout;
        cmd(&ctx);
    }
    printf("\n");

    // Clean up
    printf("Unmounting disk... ");
    err = disk_umount();
    if (err)
        printf("failed (%i)\n", err);
    else
        printf("ok\n");
    printf("Finished!\n");

}