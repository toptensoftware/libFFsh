#include "common.h"

#include "path.h"
#include "utf.h"

/*
// Check for characters allows in the "drive:" part of path
bool isDriveChar(uint32_t ch)
{
    if (ch >= '0' && ch <= '9')
        return true;
    if (ch >= 'a' && ch <= 'z')
        return true;
    if (ch >= 'A' && ch <= 'Z')
        return true;
    return false;
}

// Skip the drive letter part of a path
// Returns pointer to first character after the ':' or
// the passed in string if not found
const char* pathskipdrive(const char* path)
{
    const char* p = path;
    while (true)
    {
        uint32_t cp = utf8_decode(&p);
        if (!isDriveChar(cp))
        {
            if (cp == ':')
            {
                return p;
            }
            else
            {
                return path;
            }
        }
    }
}

*/

// Find the last separator in a path
const char* pathtail(const char* path)
{
    const char* pszSep = NULL;

    while (true)
    {
        const char* pprev = path;
        uint32_t cp = utf8_decode(&path);
        if (cp == '/' || cp == '\\' || cp == ':')
        {
            pszSep = pprev;
        }
        else if (cp == 0)
            break;
    }

    return pszSep;
}

// Find the base name part of a file path
const char* pathbase(const char* path)
{
    const char* p = pathtail(path);
    if (p == NULL)
        return NULL;
    utf8_decode(&p);
    return p;
}

// Split a path at last element, returning pointer to base name
char* pathsplit(char* path)
{
    // Find last element
    char* p = (char*)pathtail(path);
    if (!p)
        return NULL;

    // Return character after separator
    char* pBase = p;
    utf8_decode((const char**)&pBase);

    // Truncate it
    *p = '\0';

    return pBase;
}

// Copy the directory part of a path
void pathdir(const char* path, char* dir)
{
    char* p = (char*)pathtail(path);
    if (!p)
    {
        dir[0] = '\0';
        return;
    }
    memcpy(dir, path, p - path);
    dir[p-path] = '\0';
}


// Concatenate two paths, ensure a single slash/backslash between
char* pathcat(char* path1, const char* path2)
{
    // Redundant?
    if (*path2 == '\0')
        return path1;

    // Absolute path?
    /*
    const char* pszSkipDrive = pathskipdrive(path2);
    if (pszSkipDrive != path2 || *pszSkipDrive == '\\' || *pszSkipDrive == '/')
    */
    if (*path2 == '\\' || *path2 == '/')
    {
        strcpy(path1, path2);
        return path1;
    }

    // Find end of string
    char* p = path1 + strlen(path1);

    // Ensure it ends in a slash
    if (p[-1] != '/' && p[-1] != '\\')
    {
        *p++ = '/';
    }

    // Append other path
    strcpy(p, path2);

    // Return it
    return path1;
}

// Find the extension part of a path (including the '.')
const char* pathext(const char* path)
{
    const char* p = pathtail(path);

    const char* pszExt = NULL;
    while (true)
    {
        const char* pprev = p;
        uint32_t cp = utf8_decode(&p);
        if (cp == '.')
            pszExt = pprev;
        if (!cp)
            break;
    }
    return pszExt;
}


// Splits the extension from a path string, returning a pointer
// to the extension (without the '.') or NULL if no extension found
char* pathsplitext(char* path)
{
    // Find the extension
    char* pdot = (char*)pathext(path);
    if (!pdot)
        return NULL;
    
    char* pExt = (char*)pdot;
    utf8_decode((const char**)&pExt);
    *pdot = '\0';
    return pExt;
}


// Canonicalize a path
//   - Ensures leading slash
//   - Converts backslashes to slashes
//   - Converts repeated slash/bslash to single slash
//   - Trims trailing slashes
//   - Handles '.' and '..'
char* pathcan(char* path)
{
    // Skip the drive letter
    //path = (char*)pathskipdrive(path);
	char* ps = path;
	char* pd = path;

	// Must start with slash
    uint32_t cp = utf8_decode((const char**)&ps);
    if (cp != '/' && cp != '\\')
        return NULL;
    *pd++ = '/';    

	// Process parts
	while (true)
	{
		// Find next segment
		char* pSeg = ps;
        char* pSegEnd = ps;
        int dotCount = 0;
        int charCount = 0;
        while (true)
        {
            pSegEnd = ps;
            cp = utf8_decode((const char**)&ps);
            if (cp == 0 || cp == '/' || cp == '\\')
                break;
            charCount++;
            if (cp == '.')
                dotCount++;
        }

        if (charCount == 0)
        {
            // Ignore empty paths (consecutive //)
        }
		else if (dotCount == 1 && charCount == 1)
		{
			// Handle '.' - ignore
		}
		else if (dotCount == 2 && charCount == 2)
		{
			// Handle '..'

			// Check for ".." at root
			if (pd > path + 1)
            {
                // Rewind pd to previous segment
                *pd = '\0';
                pd = (char*)pathtail(path);
                if (pd == path)
                    pd++;
            }
		}
		else
		{
			// Copy to dest
            if (pd != path + 1)
				*pd++ = '/';
			memcpy(pd, pSeg, pSegEnd - pSeg);
			pd += pSegEnd - pSeg;
		}

        if (cp == 0)
            break;
	}

	*pd = '\0';
	return path;
}

// Checks if a path looks like a directory
// (ie: ends in '/', '/..' or '/.', or backslash equivalent)
bool pathisdir(const char* path)
{
    const char* pszBase = pathbase(path);
    if (!pszBase)
        return false;

    int charCount = 0;
    int dotCount = 0;
    for (uint32_t cp = utf8_decode(&pszBase); cp != 0; cp = utf8_decode(&pszBase))
    {
        if (cp == '.')
            dotCount++;
        charCount++;
    }

    if (charCount == 0)
        return true;
    if (charCount == 1 && dotCount == 1)
        return true;
    if (charCount == 2 && dotCount == 2)
        return true;
    return false;
}

// Ensure a path ends in slash or backslash
void pathensuredir(char* path)
{
    char* pszLast = path;
    uint32_t cpLast = '\0';
    for (uint32_t cp = utf8_decode((const char**)&path); cp != 0; cp = utf8_decode((const char**)&path))
    {
        cpLast = cp;
        pszLast = path;
    }
    if (cpLast != '/' && cpLast != '\\')
    {
        *pszLast++ = '/';
        *pszLast++ = '\0';
    }

}

// Check if path is the root path
bool pathisroot(const char* path)
{
    //path = pathskipdrive(path);
    uint32_t cp = utf8_decode(&path);
    return (cp == '/' || cp == '\\') && utf8_decode(&path) == '\0';
}

// Checks if 'path' is in 'dir'
// Assumes both paths are absolute and canonical
bool pathcontains(const char* parent, const char* child, bool caseSensitive)
{
    uint32_t cd_last = '\0';

    while (true)
    {
        // Decode character from each
        uint32_t cp = utf8_decode(&parent);
        uint32_t cc = utf8_decode(&child);
        
        // Convert case
        if (!caseSensitive)
        {
            cp = utf32_toupper(cp);
            cc = utf32_toupper(cc);
        }

        // Normalize directory separator
        if (cp == '\\')
            cp = '/';
        if (cc == '\\')
            cc = '/';

        // End of parent path?
        if (cp == '\0')
        {
            // If the parent had a trailing slash and we
            // matched up to it, then it's a match
            if (cd_last == '/')
                return true;

            // Must be end of child path, or a directory separator
            if (cc == '\0')
                return true;
            if (cc == '/' || cc == '\\')
                return true;

            // Not a child
            return false;
        }

        // If parent has a trailing slash but child doesn't
        // it counts as a match
        if (cp == '/' && cc == '\0')
        {
            return utf8_decode(&parent) == 0;
        }

        // Match?
        if (cp != cc)
            return false;

        // Remember the last character
        cd_last = cp;
    }
}

static uint32_t default_globchars[2] = { '*' , '?' };

#define globchar_star 0
#define globchar_question 1

// Check if path contains wildcard characters '*' or '?'
bool pathisglob(const char* path, const uint32_t* charset)
{
    if (!path)
        return false;

    if (charset == NULL)
        charset = default_globchars;
    
    for (uint32_t cp = utf8_decode(&path); cp != 0; cp = utf8_decode(&path))
    {
        if (cp == charset[globchar_star] || cp == charset[globchar_question])
            return true;
    }

    return false;
}



bool pathglob(const char* filename, const char* pattern, bool caseSensitive, const uint32_t* charset)
{
    if (charset == NULL)
        charset = default_globchars;
    
    const char* f = filename;
    const char* p = pattern;

    // Compare characters
    while (true)
    {
        const char* prevf = f;
        // Decode a character from each
        uint32_t cf = utf8_decode(&f);
        uint32_t cp = utf8_decode(&p);

        // End of both strings = match!
        if (cp=='\0' && cf=='\0')
            return true;

        // End of sub-pattern?
        if (cp=='\0' || cf=='\0')
            return cp == charset[globchar_star];

        // Single character wildcard
        if (cp==charset[globchar_question])
            continue;

        // Multi-character wildcard
        if (cp==charset[globchar_star])
        {
            // If nothing after the wildcard then match
            const char* prevp = p;
            cp = utf8_decode(&p);
            if (cp == '\0')
                return true;

            p = prevp;
            f = prevf;

            while (cf != '\0')
            {
                if (pathglob(f, p, caseSensitive, charset))
                    return true;

                cf = utf8_decode(&f);
            }
            return false;
        }

        // Handle case sensitivity
        if (!caseSensitive)
        {
            cp = utf32_toupper(cp);
            cf = utf32_toupper(cf);
        }

        // Characters match
        if (cp != cf)
            return false;
    }
}
