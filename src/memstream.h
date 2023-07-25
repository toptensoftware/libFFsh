#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Memory stream
struct MEMSTREAM
{
    bool owned;     // True if buffer malloced and owned by stream
    void* p;        // Buffer
    size_t cb;      // Size of buffer
    size_t offs;    // Current position
    size_t length;  // Length of used part of buffer
    bool error;    // True if attempted to write and failed
};

// Initialize a memory stream using a supplied block of memory.
void memstream_initfrom(struct MEMSTREAM* stream, void* p, size_t cb, size_t length, bool takeOwnership);

// Initialize a new memory stream
void memstream_initnew(struct MEMSTREAM* stream, size_t initial_size);

// Release an allocated and owned memory stream
void memstream_close(struct MEMSTREAM* stream);

// Reset memory stream to zero length, seek to start but keep allocated memory
void memstream_reset(struct MEMSTREAM* stream);

// Write data to a memory stream
size_t memstream_write(struct MEMSTREAM* stream, void* p, size_t cb);

// Read data from a memory stream
size_t memstream_read(struct MEMSTREAM* stream, void* p, size_t cb);

// Reserve space in memory stream and return pointer
void* memstream_reserve(struct MEMSTREAM* stream, size_t cb);

// Reserve space in memory stream and return pointer
void* memstream_alloc(struct MEMSTREAM* stream, size_t cb);

// Advance position in memory stream, return pointer to advanced over buffer
void* memstream_advance(struct MEMSTREAM* stream, size_t cb, size_t* pcb);

// Detach memory block from memory stream
void* memstream_detach(struct MEMSTREAM* stream);

// Get the length of memory stream
size_t memstream_length(struct MEMSTREAM* stream);

// Get the size of the memory stream buffer
size_t memstream_size(struct MEMSTREAM* stream);

// Seek
size_t memstream_seek(struct MEMSTREAM* stream, size_t offs);

// Tell 
size_t memstream_tell(struct MEMSTREAM* stream);

// Check if any errors writing to stream
bool memstream_error(struct MEMSTREAM* stream);

// Clear write error flag
void memstream_clear_error(struct MEMSTREAM* stream);

// Get pointer from offset
void* memstream_get_pointer(struct MEMSTREAM* stream, size_t offset);

// Get offset from pointer
size_t memstream_get_offset(struct MEMSTREAM* stream, void* ptr);
