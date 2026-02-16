// win_min_fwd.h
#pragma once

// Minimal Win32 forward declarations for headers.
// Does NOT include <windows.h>
// Safe to include in public headers.

#include <cstdint>

#if !defined(_WIN32) && !defined(_WIN64)
#error This header is Windows-only.
#endif

// If Windows headers were already included, do nothing.
#if !defined(_WINDEF_) && !defined(_WINNT_)

// -----------------------------------------------------------------------------
// Core scalar types
// -----------------------------------------------------------------------------

using BOOL = int;

using BYTE = unsigned char;
using WORD = unsigned short;
using DWORD = unsigned long;

using CHAR = char;
using WCHAR = wchar_t;

using INT = int;
using UINT = unsigned int;

using LONG = long; // 32-bit on Windows
using ULONG = unsigned long;

// Pointer-sized types
using INT_PTR = std::intptr_t;
using UINT_PTR = std::uintptr_t;
using LONG_PTR = std::intptr_t;
using ULONG_PTR = std::uintptr_t;

// Win32 message/result types
using LPARAM = LONG_PTR;
using WPARAM = UINT_PTR;
using LRESULT = LONG_PTR;

// -----------------------------------------------------------------------------
// HRESULT (from winnt.h)
// -----------------------------------------------------------------------------

using HRESULT = LONG;

// -----------------------------------------------------------------------------
// Opaque handles
// -----------------------------------------------------------------------------

struct HWND__;
struct HINSTANCE__;
struct HMODULE__;
struct HICON__;
struct HMENU__;
struct HBRUSH__;
struct HFONT__;
struct HPEN__;
struct HBITMAP__;
struct HRGN__;
struct HDC__;
struct HWND__;

using HINSTANCE = HINSTANCE__ *;
using HMODULE = HINSTANCE;

using HICON = HICON__ *;
using HMENU = HMENU__ *;
using HCURSOR = HICON;

using HBRUSH = HBRUSH__ *;
using HFONT = HFONT__ *;
using HPEN = HPEN__ *;
using HBITMAP = HBITMAP__ *;
using HRGN = HRGN__ *;

using HDC = HDC__ *;
using HANDLE = void *;
using HWND = HWND__ *;

// -----------------------------------------------------------------------------
// String pointer types
// -----------------------------------------------------------------------------

using LPSTR = CHAR *;
using LPCSTR = const CHAR *;

using LPWSTR = WCHAR *;
using LPCWSTR = const WCHAR *;

using LPTSTR = WCHAR *;
using LPCTSTR = const WCHAR *;

// Generic pointer aliases
using PVOID = void *;
using LPCVOID = const void *;

// Calling conventions used by Win32 callback declarations.
#ifndef WINAPI
#define WINAPI __stdcall
#endif

#ifndef CALLBACK
#define CALLBACK __stdcall
#endif

// Forward declarations for Win32 structs used by public headers.
struct tagPOINT;
using POINT = tagPOINT;

struct tagRECT;
using RECT = tagRECT;

struct tagBITMAPINFOHEADER;
using BITMAPINFOHEADER = tagBITMAPINFOHEADER;

#endif // guards
