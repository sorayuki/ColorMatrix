/*
 * Minimal runtime glue for the reproducible LLVM cross-build in build/.
 * This file is only used when COLORMATRIX_PORTABLE_BUILD is defined.
 */
#include <windows.h>
#include <stdio.h>

extern "C" int _fltused = 0;

void *operator new(size_t size)
{
    return malloc(size ? size : 1);
}

void *operator new[](size_t size)
{
    return malloc(size ? size : 1);
}

void operator delete(void *ptr)
{
    free(ptr);
}

void operator delete[](void *ptr)
{
    free(ptr);
}

void operator delete(void *ptr, size_t)
{
    free(ptr);
}

void operator delete[](void *ptr, size_t)
{
    free(ptr);
}

extern "C" BOOL WINAPI DllMain(HMODULE, DWORD, LPVOID)
{
    return TRUE;
}
