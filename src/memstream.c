#include "memstream.h"
#include <malloc.h>
#include <string.h>

// Initialize a memory stream using a supplied block of memory.
void memstream_initfrom(struct MEMSTREAM* stream, void* p, size_t cb, size_t length, bool takeOwnership)
{
    stream->p = p;
    stream->cb = cb;
    stream->owned = takeOwnership;
    stream->offs = 0;
    stream->length = length;
    stream->error = false;
}


// Initialize a new memory stream
void memstream_initnew(struct MEMSTREAM* stream, size_t initial_size)
{
    stream->p = malloc(initial_size);
    stream->cb = initial_size;
    stream->owned = true;
    stream->offs = 0;
    stream->length = 0;
    stream->error = false;
}

// Release an allocated and owned memory stream
void memstream_close(struct MEMSTREAM* stream)
{
    if (stream->owned)
    {
        stream->owned = false;
        stream->cb = 0;
        free(stream->p);
    }
}

// Reset memory stream to zero length, seek to start but keep allocated memory
void memstream_reset(struct MEMSTREAM* stream)
{
    stream->offs = 0;
    stream->length = 0;
    stream->error = false;
}

// Write data to a memory stream
size_t memstream_write(struct MEMSTREAM* stream, void* p, size_t cb)
{
    void* mem = memstream_alloc(stream, cb);
    if (!mem)
        return 0;

    memcpy(mem, p, cb);
    return cb;
}

// Read data from a memory stream
size_t memstream_read(struct MEMSTREAM* stream, void* p, size_t cb)
{
    size_t cbRead;
    void* mem = memstream_advance(stream, cb, &cbRead);
    memcpy(p, mem, cbRead);
    return cbRead;
}

// Reserve space in memory stream and return pointer
void* memstream_reserve(struct MEMSTREAM* stream, size_t cb)
{
    // Check room?
    if (stream->offs + cb > stream->cb)
    {
        // If owned grow buffer
        if (stream->owned)
        {
            // Workout how much to resize to 
            size_t newSize = stream->cb < 0x10000 ? stream->cb * 2 : stream->cb + 0x10000;
            if (stream->offs + cb > newSize)
                newSize = stream->offs + cb;

            // Resize
            void* pNewMem = realloc(stream->p, newSize);
            if (!pNewMem)
            {
                stream->error = true;
                return NULL;
            }
            stream->p = pNewMem;
            stream->cb = newSize;
        }
        else
        {
            // Don't own buffer, can't resize
            stream->error = true;
            return NULL;
        }
    }

    // Return position
    void* r = (char*)(stream->p) + stream->offs;
    if (stream->offs + cb > stream->length)
        stream->length = stream->offs + cb;
    return r;
}

// Allocate space in memory stream and return pointer
void* memstream_alloc(struct MEMSTREAM* stream, size_t cb)
{
    void* r = memstream_reserve(stream, cb);
    if (r == NULL)
        return NULL;

    stream->offs += cb;

    return r;
}

// Advance position in memory stream, return pointer to advanced over buffer
void* memstream_advance(struct MEMSTREAM* stream, size_t cb, size_t* pcb)
{
    // Check room?
    if (stream->offs + cb > stream->length)
    {
        if (stream->offs > stream->length)
        {
            *pcb = 0;
            return NULL;
        }
        cb = stream->length - stream->offs;
    }

    *pcb = cb;

    // Update position, return buffer
    void* r = (char*)(stream->p) + stream->offs;
    stream->offs += cb;
    return r;
}

// Detach memory block from memory stream
void* memstream_detach(struct MEMSTREAM* stream)
{
    void* p = stream->p;
    stream->owned = false;
    stream->p = NULL;
    stream->cb = 0;
    return p;
}

// Get the length of memory stream
size_t memstream_length(struct MEMSTREAM* stream)
{
    return stream->length;
}

// Get the size of the memory stream buffer
size_t memstream_size(struct MEMSTREAM* stream)
{
    return stream->cb;
}

// Seek
size_t memstream_seek(struct MEMSTREAM* stream, size_t offs)
{
    size_t old = stream->offs;
    stream->offs = offs;
    return old;
}

// Tell 
size_t memstream_tell(struct MEMSTREAM* stream)
{
    return stream->offs;
}

// Check if any errors writing to stream
bool memstream_error(struct MEMSTREAM* stream)
{
    return stream->error;
}

// Clear write error flag
void memstream_clear_error(struct MEMSTREAM* stream)
{
    stream->error = false;
}

// Get pointer from offset
void* memstream_get_pointer(struct MEMSTREAM* stream, size_t offset)
{
    return (char*)(stream->p) + offset;
}

// Get offset from pointer
size_t memstream_get_offset(struct MEMSTREAM* stream, void* ptr)
{
    return (char*)ptr - (char*)(stream->p);
}
