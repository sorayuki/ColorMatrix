#ifndef COLORMATRIX_COMPAT_PROCESS_H
#define COLORMATRIX_COMPAT_PROCESS_H
#include <windef.h>
typedef unsigned (__stdcall *CM_BEGINTHREAD_PROC)(void *);
extern "C" __declspec(dllimport) ULONG_PTR __cdecl _beginthreadex(void *, unsigned, CM_BEGINTHREAD_PROC, void *, unsigned, unsigned *);
#endif
