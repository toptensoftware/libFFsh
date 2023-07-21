#include <stdio.h>
#include <alloca.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#define __USE_MISC
#include <unistd.h>

#include "diskio.h"
#include "../ff15/source/ff.h"
#include "../src/commands.h"
#include "../src/path.h"
#include "../src/bracexp.h"

void ffsh_printf(void (*write)(void*, char), void* arg, const char* format, ...)
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

void ffsh_sleep(uint32_t millis)
{
    usleep(millis * 1000);
}

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

        // Read command
        char szCommand[512];
        if (!fgets(szCommand, sizeof(szCommand), stdin))
            break;
        char* psz = szCommand;
        while (*psz != '\n' && *psz != '\0')
            psz++;
        *psz = '\0';

        // Setup command context
        FFSH_CONTEXT ctx;
        ctx.cwd = cwd;
        ctx.user = NULL;
        ctx.pfn_stderr = pfn_stderr;
        ctx.pfn_stdout = pfn_stdout;
        ffsh_exec(&ctx, szCommand);

        if (ctx.did_exit)
            break;
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