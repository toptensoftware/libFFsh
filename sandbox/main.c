#include "common.h"
#include "diskio.h"
#include <alloca.h>


int main()
{
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