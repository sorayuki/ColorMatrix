#ifndef COLORMATRIX_COMPAT_WINDEF_H
#define COLORMATRIX_COMPAT_WINDEF_H
#ifndef NULL
#define NULL 0
#endif
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef int BOOL;
typedef void *HANDLE;
typedef void *HMODULE;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef __SIZE_TYPE__ size_t;
typedef __SIZE_TYPE__ SIZE_T;
typedef __UINTPTR_TYPE__ ULONG_PTR;
typedef ULONG_PTR DWORD_PTR;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#endif
