#pragma once

#include <stdbool.h>

//const char* pathskipdrive(const char* path);
const char* pathtail(const char* path);
const char* pathbase(const char* path);
void pathdir(const char* path, char* dir);
char* pathsplit(char* path);
char* pathsplitext(char* path);
char* pathcat(char* path1, const char* path2);
const char* pathext(const char* path);
char* pathcan(char* path);
bool pathisdir(const char* path);
void pathensuredir(char* path);
bool pathisroot(const char* path);
bool pathcontains(const char* dir, const char* path, bool caseSensitive);
bool pathisglob(const char* path, const uint32_t* charset);
bool pathglob(const char* filename, const char* pattern, bool caseSensitive, const uint32_t* charset);
