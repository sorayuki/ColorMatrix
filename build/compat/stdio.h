#ifndef COLORMATRIX_COMPAT_STDIO_H
#define COLORMATRIX_COMPAT_STDIO_H
#include <windef.h>
typedef struct _iobuf FILE;
extern "C" {
__declspec(dllimport) int __cdecl sprintf(char *, const char *, ...);
__declspec(dllimport) int __cdecl sscanf(const char *, const char *, ...);
__declspec(dllimport) FILE * __cdecl fopen(const char *, const char *);
__declspec(dllimport) int __cdecl fclose(FILE *);
__declspec(dllimport) char * __cdecl fgets(char *, int, FILE *);
__declspec(dllimport) int __cdecl strncmp(const char *, const char *, SIZE_T);
__declspec(dllimport) void * __cdecl malloc(SIZE_T);
__declspec(dllimport) void * __cdecl calloc(SIZE_T, SIZE_T);
__declspec(dllimport) void __cdecl free(void *);
__declspec(dllimport) int __cdecl abs(int);
__declspec(dllimport) void * __cdecl memset(void *, int, SIZE_T);
__declspec(dllimport) void * __cdecl memcpy(void *, const void *, SIZE_T);
}
#endif
