#ifndef RF_PLATFORM_COMPAT_H
#define RF_PLATFORM_COMPAT_H

/*
 * Single operating-system boundary for the native runtime port. Gameplay
 * sources retain their historical calls while adapters provide POSIX/SDL
 * implementations. This header intentionally declares no fake DirectSound or
 * Video-for-Windows behavior; those subsystems receive dedicated adapters.
 */
#include <ctype.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

/* Byte-packed little-endian data must not be loaded through WORD pointers. */
static inline uint16_t rf_read_le16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

#ifdef __cplusplus
#include <fstream>
#include <iostream>
#include <new>
#include <string>

using std::ifstream;
using std::istream;
using std::ofstream;
using std::ostream;
using std::cout;
using std::endl;
#endif

typedef uint8_t BYTE;
typedef BYTE *LPBYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef int32_t BOOL;
typedef int32_t INT;
typedef int16_t SHORT;
typedef uint32_t UINT;
typedef uint32_t MMRESULT;
typedef int64_t __int64;
typedef uint32_t ULONG;
typedef uint16_t ATOM;
typedef float FLOAT;
typedef uintptr_t DWORD_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t LPARAM;
typedef uintptr_t WPARAM;
typedef intptr_t LRESULT;
typedef void *HANDLE;
typedef void *HWND;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef void *HDC;
typedef void *HGLRC;
typedef void *HMMIO;
typedef void *HMIXER;
typedef void *HBRUSH;
typedef void *HCURSOR;
typedef void *HICON;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef const char *LPCTSTR;
typedef char TCHAR;
typedef void *LPVOID;
typedef uint32_t FOURCC;
typedef LONG HRESULT;

typedef struct tagPOINT { LONG x, y; } POINT;
typedef struct tagSIZE { LONG cx, cy; } SIZE;
typedef struct tagRECT { LONG left, top, right, bottom; } RECT;
typedef struct tagMSG {
    void *hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
} MSG;

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef struct tagWNDCLASS {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    const char *lpszMenuName;
    const char *lpszClassName;
} WNDCLASS;

typedef struct tagOPENFILENAME {
    DWORD lStructSize;
    HWND hwndOwner;
    HINSTANCE hInstance;
    const char *lpstrFilter;
    char *lpstrCustomFilter;
    DWORD nMaxCustFilter;
    DWORD nFilterIndex;
    char *lpstrFile;
    DWORD nMaxFile;
    char *lpstrFileTitle;
    DWORD nMaxFileTitle;
    const char *lpstrInitialDir;
    const char *lpstrTitle;
    DWORD Flags;
    WORD nFileOffset;
    WORD nFileExtension;
    const char *lpstrDefExt;
    LPARAM lCustData;
    void *lpfnHook;
    const char *lpTemplateName;
} OPENFILENAME;

typedef struct tWAVEFORMATEX {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} WAVEFORMATEX, *LPWAVEFORMATEX;

typedef struct _MMCKINFO {
    FOURCC ckid;
    DWORD cksize;
    FOURCC fccType;
    DWORD dwDataOffset;
    DWORD dwFlags;
} MMCKINFO;

typedef void (*LPTIMECALLBACK)(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

struct IDirectSound;
struct IDirectSoundBuffer;
typedef struct IDirectSound *LPDIRECTSOUND;
typedef struct IDirectSoundBuffer *LPDIRECTSOUNDBUFFER;
typedef void *PAVIFILE;
typedef void *PAVISTREAM;
typedef void *PGETFRAME;

#define WINAPI
#define CALLBACK
#define APIENTRY
#define __cdecl
#define __stdcall
#define _cdecl
#define FAR

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 4096
#endif
#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif

#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _getcwd getcwd
#define _chdir chdir
#define _mkdir(path) mkdir((path), 0777)
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _access access
#define _strdup strdup

#define MB_OK 0u
#define MB_ICONSTOP 0u
#define MB_ICONERROR 0u
#define MB_ICONWARNING 0u
#define MB_YESNO 0u
#define IDYES 1
#define IDNO 0
#define SW_SHOW 1
#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_MINIMIZE 6
#define PM_NOREMOVE 0u
#define WM_COMMAND 0x0111u
#define WM_INITDIALOG 0x0110u
#define WM_TIMER 0x0113u
#define WM_SETFOCUS 0x0007u
#define WM_KILLFOCUS 0x0008u
#define WM_WINDOWPOSCHANGED 0x0047u
#define CS_VREDRAW 0x0001u
#define CS_HREDRAW 0x0002u
#define WS_CHILD 0x40000000u
#define WS_POPUP 0x80000000u
#define CW_USEDEFAULT ((int)0x80000000u)
#define SWP_NOSIZE 0x0001u
#define SWP_NOZORDER 0x0004u
#define SWP_NOCOPYBITS 0x0100u
#define SWP_SHOWWINDOW 0x0040u
#define GWL_STYLE (-16)
#define LB_ADDSTRING 0x0180u
#define LB_GETCURSEL 0x0188u
#define LB_GETITEMDATA 0x0199u
#define LB_SETITEMDATA 0x019Au
#define LB_RESETCONTENT 0x0184u
#define LB_SETCURSEL 0x0186u
#define LB_SETHORIZONTALEXTENT 0x0194u
#define LBN_SELCHANGE 1u
#define LBN_DBLCLK 2u
#define IDOK 1
#define IDCANCEL 2
#define IDC_ARROW ((const char *)32512)
#define OFN_EXPLORER 0x00080000u
#define BLACK_BRUSH 4
#define MB_ICONEXCLAMATION 0x00000030u
#define HWND_TOP ((HWND)(intptr_t)0)
#define HWND_TOPMOST ((HWND)(intptr_t)-1)
#define MAKEINTRESOURCE(value) ((const char *)(uintptr_t)(WORD)(value))
#define LOWORD(value) ((WORD)((uintptr_t)(value) & 0xffffu))
#define HIWORD(value) ((WORD)(((uintptr_t)(value) >> 16) & 0xffffu))

/* Win32 virtual-key values retained by historical configuration files. */
#define VK_LBUTTON 0x01
#define VK_RBUTTON 0x02
#define VK_MBUTTON 0x04
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_ADD 0x6B
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define SM_SWAPBUTTON 23

#define MIXERLINE_COMPONENTTYPE_DST_SPEAKERS 4u
#define MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER 0x1004u
#define MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC 0x1005u
#define MIXERLINE_COMPONENTTYPE_SRC_LAST 0x100Au
#define MIXERCONTROL_CONTROLTYPE_VOLUME 0x50030001u
#define TIME_PERIODIC 0x0001u
#define TIME_CALLBACK_FUNCTION 0x0000u

typedef pthread_mutex_t CRITICAL_SECTION;
static inline void InitializeCriticalSection(CRITICAL_SECTION *section)
{
    pthread_mutex_init(section, NULL);
}
static inline void DeleteCriticalSection(CRITICAL_SECTION *section)
{
    pthread_mutex_destroy(section);
}
static inline void EnterCriticalSection(CRITICAL_SECTION *section)
{
    pthread_mutex_lock(section);
}
static inline void LeaveCriticalSection(CRITICAL_SECTION *section)
{
    pthread_mutex_unlock(section);
}

#ifdef __cplusplus
extern "C" {
#endif

int MessageBox(void *window, const char *text, const char *caption,
               unsigned int type);
void OutputDebugString(const char *text);
int ShowCursor(int show);
void Sleep(DWORD milliseconds);
DWORD GetTickCount(void);
DWORD timeGetTime(void);
SHORT GetAsyncKeyState(int virtual_key);
int GetSystemMetrics(int index);
BOOL PeekMessage(MSG *message, HWND window, UINT first, UINT last,
                 UINT remove_message);
BOOL GetMessage(MSG *message, HWND window, UINT first, UINT last);
BOOL TranslateMessage(const MSG *message);
LRESULT DispatchMessage(const MSG *message);
BOOL GetClientRect(HWND window, RECT *rect);
BOOL ClientToScreen(HWND window, POINT *point);
BOOL GetCursorPos(POINT *point);
BOOL SetCursorPos(int x, int y);
HWND GetDlgItem(HWND dialog, int item);
HDC GetDC(HWND window);
int ReleaseDC(HWND window, HDC dc);
LRESULT SendDlgItemMessage(HWND dialog, int item, UINT message,
                           WPARAM wparam, LPARAM lparam);
BOOL GetTextExtentPoint32(HDC dc, const char *text, int length, SIZE *size);
BOOL GetWindowRect(HWND window, RECT *rect);
HWND GetDesktopWindow(void);
BOOL SetWindowPos(HWND window, HWND insert_after, int x, int y, int width,
                  int height, UINT flags);
BOOL UpdateWindow(HWND window);
BOOL ShowWindow(HWND window, int command);
LONG GetWindowLong(HWND window, int index);
BOOL AdjustWindowRect(RECT *rect, DWORD style, BOOL menu);
HWND SetFocus(HWND window);
intptr_t DialogBoxParam(HINSTANCE instance, const char *template_name,
                        HWND parent, DLGPROC callback, LPARAM parameter);
BOOL EndDialog(HWND dialog, intptr_t result);
ATOM RegisterClass(const WNDCLASS *window_class);
HICON LoadIcon(HINSTANCE instance, const char *name);
HCURSOR LoadCursor(HINSTANCE instance, const char *name);
HWND CreateWindowEx(DWORD extended_style, const char *class_name,
                    const char *window_name, DWORD style, int x, int y,
                    int width, int height, HWND parent, void *menu,
                    HINSTANCE instance, void *parameter);
BOOL DestroyWindow(HWND window);
LRESULT DefWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
BOOL GetOpenFileName(OPENFILENAME *open_file);
BOOL GetKeyboardState(BYTE *state);
int ToAscii(UINT virtual_key, UINT scan_code, const BYTE *key_state,
            WORD *characters, UINT flags);
void *GetStockObject(int object);
UINT_PTR SetTimer(HWND window, UINT_PTR event_id, UINT interval,
                  void *callback);
void AVIFileInit(void);
void AVIFileExit(void);
char *_strupr(char *text);
char *itoa(int value, char *buffer, int radix);
#define _P_WAIT 0
intptr_t _spawnv(int mode, const char *path, const char *const *arguments);
MMRESULT timeSetEvent(UINT delay, UINT resolution, LPTIMECALLBACK callback,
                      DWORD_PTR user, UINT event_type);
MMRESULT timeKillEvent(UINT timer_id);
char *rf_fgets(char *buffer, int size, FILE *stream);
void rf_fatal_exit(int code) __attribute__((noreturn));
/* 1 resolved, 0 absent, -1 ambiguous/invalid. Never chooses arbitrarily. */
int rf_case_path(const char *path, char *resolved, size_t capacity, int route);

#ifdef __cplusplus
}
#endif

#ifndef RF_PLATFORM_NATIVE_STDIO
#define fgets rf_fgets
#define exit(n) rf_fatal_exit(n)
#endif

#endif
