#include "common.h"

#include "ffex.h"
#include "path.h"

FRESULT f_stat_ex(
    const TCHAR* path,	/* Pointer to the file path */
	FILINFO* fno		/* Pointer to file information to return */
)
{
    if (pathisroot(path))
    {
        fno->fdate = 0;
        fno->ftime = 0;
        fno->fsize = 0;
        fno->fattrib = AM_DIR;
        strcpy(fno->fname, "/");
        return FR_OK;
    }

    return f_stat(path, fno);
}


// Map a FatFS error code to standard error code
int f_maperr(int err)
{
    return err + 1000;
}


const char* f_strerror(int err)
{
    switch (err)
    {
        case FR_OK: return "Succeeded";
        case FR_DISK_ERR: return "a hard error occurred in the low level disk I/O layer";
        case FR_INT_ERR: return "assertion failed";
        case FR_NOT_READY: return "the physical drive cannot work";
        case FR_NO_FILE: return "could not find the file";
        case FR_NO_PATH: return "could not find the path";
        case FR_INVALID_NAME: return "the path name format is invalid";
        case FR_DENIED: return "access denied due to prohibited access or directory full";
        case FR_EXIST: return "access denied due to prohibited access";
        case FR_INVALID_OBJECT: return "the file/directory object is invalid";
        case FR_WRITE_PROTECTED: return "the physical drive is write protected";
        case FR_INVALID_DRIVE: return "the logical drive number is invalid";
        case FR_NOT_ENABLED: return "the volume has no work area";
        case FR_NO_FILESYSTEM: return "there is no valid FAT volume";
        case FR_MKFS_ABORTED: return "the f_mkfs() aborted due to any problem";
        case FR_TIMEOUT: return "could not get a grant to access the volume within defined period";
        case FR_LOCKED: return "the operation is rejected according to the file sharing policy";
        case FR_NOT_ENOUGH_CORE: return "LFN working buffer could not be allocated";
        case FR_TOO_MANY_OPEN_FILES: return "too many open files";
        case FR_INVALID_PARAMETER: return "given parameter is invalid";
    }
    return "unknown error";
}


bool f_is_hidden(FILINFO* pfi)
{
    return (pfi->fattrib & AM_HID) || (pfi->fname[0] == '.');
}


// Copy a file
int f_copyfile(const char* pszDest, const char* pszSrc, bool optOverwrite, void (*progress)())
{
    // Check destination
    FILINFO fiDest;
    int err = f_stat(pszDest, &fiDest);
    if (err == 0)
    {
        // If overwrite disabled, ignore
        if (!optOverwrite)
            return 0;
        
        // If target is a directory then fail
        if (fiDest.fattrib & AM_DIR)
            return FR_EXIST;
    }

    // Open source file
    FIL src;
    err = f_open(&src, pszSrc, FA_OPEN_EXISTING | FA_READ);
    if (err)
        return err;

    // Open target file
    FIL dest;
    err = f_open(&dest, pszDest, FA_CREATE_ALWAYS | FA_WRITE);
    if (err)
    {
        f_close(&src);
        return err;
    }

    // Copy
    char buf[512];
    while (true)
    {
        // Read
        UINT bytes_read;
        err = f_read(&src, buf, sizeof(buf), &bytes_read);
        if (err)
            goto fail;

        // Write
        UINT bytes_written;
        err = f_write(&dest, buf, bytes_read, &bytes_written);
        if (err)
            goto fail;

        // EOF?
        if (bytes_read < sizeof(buf))
            break;

        if (progress)
            progress();
    }

    // Close files
    f_close(&src);
    f_close(&dest);

    // Get source stat
    FILINFO fi;
    err = f_stat(pszSrc, &fi);
    if (err)
        goto fail;

    // Update timestamp
    err = f_utime(pszDest, &fi);
    if (err)
        goto fail;

    // Update attributes
    err = f_chmod(pszDest, fi.fattrib, AM_ARC|AM_HID|AM_RDO|AM_SYS);
    if (err)
        goto fail;
    
    // Success!
    return 0;

fail:
    f_close(&src);
    f_close(&dest);
    f_unlink(pszDest);
    return err;
}


// Recursively remove a directory
// psz buffer should be at least FF_MAX_LFN in size
int f_rmdir_r(char* psz, void (*progress)())
{
    // Otherwise enumerate the directory, deleting all sub items
    DIR dir;
    int err = f_opendir(&dir, psz);
    if (err)
        return err;

    // Remember end of passed path
    char* pszEnd = psz + strlen(psz);
    bool dirOpen = true;

    // Delete all directory entries
    while (true)
    {
        if (progress)
            progress();
            
        FILINFO fi;
        int err = f_readdir(&dir, &fi);
        if (err)
        {
            f_closedir(&dir);
            return err;
        }

        // Nothing more?
        if (fi.fname[0] == '\0')
            break;

        // Get full name
        pathcat(psz, fi.fname);

        // Delete file, recurse sub-directory
        if (fi.fattrib & AM_DIR)
        {
            err = f_rmdir_r(psz, progress);

            // If too many files error, then close our
            // directory, remove the sub-directory then
            // re-open
            if (err == FR_TOO_MANY_OPEN_FILES)
            {
                f_closedir(&dir);
                dirOpen = false;

                err = f_rmdir_r(psz, progress);

                if (err == 0)
                {
                    *pszEnd = '\0';
                    err = f_opendir(&dir, psz);
                    dirOpen = err == 0;
                }
            }
        }
        else
        {
            err = f_unlink(psz);
        }

        *pszEnd = '\0';

        if (err)
        {
            if (dirOpen)
                f_closedir(&dir);
            return err;
        }
    }

    if (dirOpen)
        f_closedir(&dir);

    // Remove the now empty directory
    return f_rmdir(psz);
}



int f_mkdir_r(const char* psz)
{
    // Is it the root directory
    if (pathisroot(psz))
        return 0;

    // Try to create it
    int err = f_mkdir(psz);
    if (err == 0 || err != FR_NO_PATH)
        return 0;

    // Find base
    char* base = (char*)pathbase(psz);
    if (!base)
        return ENOENT;

    // Make parent
    char chSave = base[-1];
    base[-1] = '\0';
    err = f_mkdir_r(psz);
    base[-1] = chSave;
    if (err)
        return err;

    // Try again
    return f_mkdir(psz);
}

