#ifndef COLORMATRIX_COMPAT_WINDOWS_H
#define COLORMATRIX_COMPAT_WINDOWS_H
#include <windef.h>
#define WINAPI __stdcall
#define INFINITE 0xFFFFFFFFUL
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define UInt32x32To64(a,b) ((unsigned __int64)(unsigned long)(a) * (unsigned __int64)(unsigned long)(b))
#define Int64ShrlMod32(a,b) ((unsigned __int64)(a) >> (b))
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);
extern "C" {
__declspec(dllimport) DWORD WINAPI GetCurrentThreadId(void);
__declspec(dllimport) void WINAPI OutputDebugStringA(LPCSTR);
__declspec(dllimport) int WINAPI lstrcmpiA(LPCSTR, LPCSTR);
__declspec(dllimport) HANDLE WINAPI CreateEventA(LPVOID, BOOL, BOOL, LPCSTR);
__declspec(dllimport) HANDLE WINAPI CreateThread(LPVOID, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, DWORD *);
__declspec(dllimport) BOOL WINAPI SetEvent(HANDLE);
__declspec(dllimport) BOOL WINAPI ResetEvent(HANDLE);
__declspec(dllimport) DWORD WINAPI WaitForSingleObject(HANDLE, DWORD);
__declspec(dllimport) DWORD WINAPI WaitForMultipleObjects(DWORD, const HANDLE *, BOOL, DWORD);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE);
__declspec(dllimport) HANDLE WINAPI GetCurrentProcess(void);
__declspec(dllimport) BOOL WINAPI GetProcessAffinityMask(HANDLE, DWORD_PTR *, DWORD_PTR *);
__declspec(dllimport) LONG WINAPI InterlockedIncrement(LONG *);
__declspec(dllimport) LONG WINAPI InterlockedDecrement(LONG *);
}
#define OutputDebugString OutputDebugStringA
#define lstrcmpi lstrcmpiA
#define CreateEvent CreateEventA
#endif
