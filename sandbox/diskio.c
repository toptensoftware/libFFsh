#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>

#include "../ff15/source/ff.h"
#include "../ff15/source/diskio.h"

// FatFS file system object
FATFS g_fs;
FILE* g_pFile = NULL;

DSTATUS disk_initialize(BYTE pdrv)
{
    int fd = open("disk.fat", O_RDWR | O_CREAT, 0666);
    g_pFile = fdopen(fd, "r+");
    if (!g_pFile)
        return -1;
    return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
    return 0;    
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    fseek(g_pFile, sector * 512, SEEK_SET);
    if (fread(buff, 512, count, g_pFile) != count)
        return RES_ERROR;
    return 0;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    fseek(g_pFile, sector * 512, SEEK_SET);
    if (fwrite(buff, 512, count, g_pFile) != count)
        return RES_ERROR;
    return 0;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* data)
{
	if (cmd == GET_SECTOR_COUNT)
	{
		*((LBA_t*)data) = (1024 * 1024 * 100) / 512;
		return 0;
	}
	if (cmd == GET_BLOCK_SIZE)
	{
		*((DWORD*)data) = 512;
		return 0;
	}
    if (cmd == CTRL_SYNC)
    {
        fflush(g_pFile);
        return 0;
    }
    
    return -1;
    return RES_OK;    
}

DWORD get_fattime()
{
    // Get current time
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo = localtime(&rawtime);
    int year = timeinfo->tm_year + 1900;
    int month = timeinfo->tm_mon + 1;           // 1 - 12
    int day = timeinfo->tm_mday;                // 1 - 31
    int hour = timeinfo->tm_hour;               // 0 - 23    
    int minute = timeinfo->tm_min;              // 0 - 59
    int seconds = timeinfo->tm_sec;             // 0 - 59

    // Convert to DOS format
    return (
        (DWORD)(year - 1980) << 25 | 
        (DWORD)month << 21 | 
        (DWORD)day << 16 | 
        (DWORD)hour << 11 | 
        (DWORD)minute << 5 | 
        (DWORD)(seconds / 2) << 0
        );
}

bool g_bMounted = false;

int disk_mount()
{
    if (g_bMounted)
        return 0;

    // Mount it
    int err = f_mount(&g_fs, "", 1);
    if (err)
        return err;

    g_bMounted = true;

    return 0;
}


int disk_umount()
{
    f_unmount("");
    fclose(g_pFile);
    return 0;
}