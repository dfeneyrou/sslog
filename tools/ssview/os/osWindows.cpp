#ifdef _WIN32

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <shlobj_core.h>
#include <windows.h>
#include <windowsx.h>

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OS common
#define AP_IMPLEMENTATION 1
#include "asserted.h"
#include "bs.h"
#include "bsOs.h"
#define GL_WINDOWS_IMPLEMENTATION
#include <dwmapi.h>

#include "base/bsString.h"
#include "base/bsTime.h"
#include "glWindows.h"  // Before any other include, to ensure that we get the implementation
#include "keycode.h"

// Local context
static int       globWindowWidth  = -1;
static int       globWindowHeight = -1;
static HINSTANCE globHInstance    = 0;
static int       globNCmdShow     = -1;
static LPTSTR    windowClass;  // Window Class
static HGLRC     RC;           // Rendering Context
static HDC       DC;           // Device Context
static HWND      WND;          // Window
static HCURSOR   DFT_CURSOR;   // Cursor
static HCURSOR   CUR_CURSOR;
static bsString  APP_PATH;

bsKeycode
convertKeyCode(WPARAM key, LPARAM flags)
{
    switch (key) {
        case VK_SHIFT: {  // Check the scancode to distinguish between left and right
                          // shift
            static UINT lShift   = MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC);
            UINT        scancode = static_cast<UINT>((flags & (0xFF << 16)) >> 16);
            return scancode == lShift ? KC_LShift : KC_RShift;
        }
        case VK_MENU:  // Check the "extended" flag to distinguish between left and
                       // right alt
            return (HIWORD(flags) & KF_EXTENDED) ? KC_RAlt : KC_LAlt;
        case VK_CONTROL:  // Check the "extended" flag to distinguish between left and
                          // right control
            return (HIWORD(flags) & KF_EXTENDED) ? KC_RControl : KC_LControl;

        // Other keys are reported properly
        case VK_LWIN:
            return KC_LSystem;
        case VK_RWIN:
            return KC_RSystem;
        case VK_APPS:
            return KC_Menu;
        case VK_OEM_1:
            return KC_Semicolon;
        case VK_OEM_2:
            return KC_Slash;
        case VK_OEM_PLUS:
            return KC_Equal;
        case VK_OEM_MINUS:
            return KC_Hyphen;
        case VK_OEM_4:
            return KC_LBracket;
        case VK_OEM_6:
            return KC_RBracket;
        case VK_OEM_COMMA:
            return KC_Comma;
        case VK_OEM_PERIOD:
            return KC_Period;
        case VK_OEM_7:
            return KC_Quote;
        case VK_OEM_5:
            return KC_Backslash;
        case VK_OEM_3:
            return KC_Tilde;
        case VK_ESCAPE:
            return KC_Escape;
        case VK_SPACE:
            return KC_Space;
        case VK_RETURN:
            return KC_Enter;
        case VK_BACK:
            return KC_Backspace;
        case VK_TAB:
            return KC_Tab;
        case VK_PRIOR:
            return KC_PageUp;
        case VK_NEXT:
            return KC_PageDown;
        case VK_END:
            return KC_End;
        case VK_HOME:
            return KC_Home;
        case VK_INSERT:
            return KC_Insert;
        case VK_DELETE:
            return KC_Delete;
        case VK_ADD:
            return KC_Add;
        case VK_SUBTRACT:
            return KC_Subtract;
        case VK_MULTIPLY:
            return KC_Multiply;
        case VK_DIVIDE:
            return KC_Divide;
        case VK_PAUSE:
            return KC_Pause;
        case VK_F1:
            return KC_F1;
        case VK_F2:
            return KC_F2;
        case VK_F3:
            return KC_F3;
        case VK_F4:
            return KC_F4;
        case VK_F5:
            return KC_F5;
        case VK_F6:
            return KC_F6;
        case VK_F7:
            return KC_F7;
        case VK_F8:
            return KC_F8;
        case VK_F9:
            return KC_F9;
        case VK_F10:
            return KC_F10;
        case VK_F11:
            return KC_F11;
        case VK_F12:
            return KC_F12;
        case VK_F13:
            return KC_F13;
        case VK_F14:
            return KC_F14;
        case VK_F15:
            return KC_F15;
        case VK_LEFT:
            return KC_Left;
        case VK_RIGHT:
            return KC_Right;
        case VK_UP:
            return KC_Up;
        case VK_DOWN:
            return KC_Down;
        case VK_NUMPAD0:
            return KC_Numpad0;
        case VK_NUMPAD1:
            return KC_Numpad1;
        case VK_NUMPAD2:
            return KC_Numpad2;
        case VK_NUMPAD3:
            return KC_Numpad3;
        case VK_NUMPAD4:
            return KC_Numpad4;
        case VK_NUMPAD5:
            return KC_Numpad5;
        case VK_NUMPAD6:
            return KC_Numpad6;
        case VK_NUMPAD7:
            return KC_Numpad7;
        case VK_NUMPAD8:
            return KC_Numpad8;
        case VK_NUMPAD9:
            return KC_Numpad9;
        case 'A':
            return KC_A;
        case 'Z':
            return KC_Z;
        case 'E':
            return KC_E;
        case 'R':
            return KC_R;
        case 'T':
            return KC_T;
        case 'Y':
            return KC_Y;
        case 'U':
            return KC_U;
        case 'I':
            return KC_I;
        case 'O':
            return KC_O;
        case 'P':
            return KC_P;
        case 'Q':
            return KC_Q;
        case 'S':
            return KC_S;
        case 'D':
            return KC_D;
        case 'F':
            return KC_F;
        case 'G':
            return KC_G;
        case 'H':
            return KC_H;
        case 'J':
            return KC_J;
        case 'K':
            return KC_K;
        case 'L':
            return KC_L;
        case 'M':
            return KC_M;
        case 'W':
            return KC_W;
        case 'X':
            return KC_X;
        case 'C':
            return KC_C;
        case 'V':
            return KC_V;
        case 'B':
            return KC_B;
        case 'N':
            return KC_N;
        case '0':
            return KC_Num0;
        case '1':
            return KC_Num1;
        case '2':
            return KC_Num2;
        case '3':
            return KC_Num3;
        case '4':
            return KC_Num4;
        case '5':
            return KC_Num5;
        case '6':
            return KC_Num6;
        case '7':
            return KC_Num7;
        case '8':
            return KC_Num8;
        case '9':
            return KC_Num9;
    }

    return KC_Unknown;
}

#define LOAD_GLEX(glDecl, glFuncName)                                                 \
    glFuncName = reinterpret_cast<glDecl>(wglGetProcAddress(#glFuncName));            \
    if (!glFuncName) {                                                                \
        MessageBox(0, L"" #glFuncName "() failed.", L"Window::create", MB_ICONERROR); \
        return;                                                                       \
    }

static void
showMessage(LPCWSTR message)
{
    MessageBox(0, message, L"Window::create", MB_ICONERROR);
}

static LRESULT CALLBACK
WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void
osalCreateWindow()
{
    // Register the application class
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(wcex));
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wcex.lpfnWndProc   = WindowProcedure;
    wcex.hInstance     = globHInstance;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = L"Core";
    windowClass        = MAKEINTATOM(RegisterClassEx(&wcex));
    if (windowClass == 0) {
        showMessage(L"registerClass() failed.");
        return;
    }

    // create temporary window
    HWND fakeWND = CreateWindow(windowClass, L"Fake Window", WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 1,
                                1,                                 // position x, y  width, height
                                NULL, NULL, globHInstance, NULL);  // parent window, menu, instance, param
    HDC  fakeDC  = GetDC(fakeWND);                                 // Device Context

    PIXELFORMATDESCRIPTOR fakePFD;
    ZeroMemory(&fakePFD, sizeof(fakePFD));
    fakePFD.nSize      = sizeof(fakePFD);
    fakePFD.nVersion   = 1;
    fakePFD.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    fakePFD.iPixelType = PFD_TYPE_RGBA;
    fakePFD.cColorBits = 32;
    fakePFD.cAlphaBits = 8;
    fakePFD.cDepthBits = 24;

    const int fakePFDID = ChoosePixelFormat(fakeDC, &fakePFD);
    if (fakePFDID == 0) {
        showMessage(L"ChoosePixelFormat() failed.");
        return;
    }

    if (SetPixelFormat(fakeDC, fakePFDID, &fakePFD) == false) {
        showMessage(L"SetPixelFormat() failed.");
        return;
    }

    HGLRC fakeRC = wglCreateContext(fakeDC);  // Rendering Contex

    if (fakeRC == 0) {
        showMessage(L"wglCreateContext() failed.");
        return;
    }

    if (wglMakeCurrent(fakeDC, fakeRC) == false) {
        showMessage(L"wglMakeCurrent() failed.");
        return;
    }

    // get pointers to functions (or init opengl loader here)
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
    LOAD_GLEX(PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB);
    /*
        wglChoosePixelFormatARB =
       reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
        if (wglChoosePixelFormatARB == nullptr) {
                showMessage("wglGetProcAddress() failed.");
                return 1;
        }
    */

    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
    LOAD_GLEX(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB);
    /*
        wglCreateContextAttribsARB =
       reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
        if (wglCreateContextAttribsARB == nullptr) {
                showMessage("wglGetProcAddress() failed.");
                return 1;
        }
    */
    // Compute the window location & size
    RECT primaryDisplaySize;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &primaryDisplaySize,
                         0);  // system taskbar and application desktop toolbars not included
    int wPosX = primaryDisplaySize.left + WINDOW_BORDER_X, wPosY = primaryDisplaySize.top + WINDOW_BORDER_Y;
    if (!WINDOW_BORDER_DEBUG) {
        // Standard, centered window
        globWindowWidth  = (primaryDisplaySize.right - primaryDisplaySize.left) - 2 * WINDOW_BORDER_X;
        globWindowHeight = (primaryDisplaySize.bottom - primaryDisplaySize.top) - 2 * WINDOW_BORDER_Y;
    } else {
        // Debug position in the top right corner (as it may block the access to
        // other windows below...)
        globWindowWidth  = (primaryDisplaySize.right - primaryDisplaySize.left) - 2 * WINDOW_BORDER_X;
        globWindowHeight = (primaryDisplaySize.bottom - primaryDisplaySize.top) - 2 * WINDOW_BORDER_Y;
        wPosX            = 2 * WINDOW_BORDER_X - 50;
        wPosY            = 50;
    }

    // create a new window and context
    WND = CreateWindowEx(
        /*WS_EX_COMPOSITED|WS_EX_TRANSPARENT| WS_EX_TOPMOST|
           WS_EX_APPWINDOW|WS_EX_LAYERED*/
        WS_EX_APPWINDOW, windowClass, L"OpenGL Window",  // class name, window name
        // WS_CLIPSIBLINGS | WS_CLIPCHILDREN |   WS_VISIBLE | WS_OVERLAPPEDWINDOW
        // ,
        WS_VISIBLE | WS_POPUP, wPosX,
        wPosY,                              // posx, posy. If x is set to CW_USEDEFAULT y is ignored
        globWindowWidth, globWindowHeight,  // width, height
        NULL, NULL,                         // parent window, menu
        globHInstance, NULL);               // instance, param
    if (!WND) {
        showMessage(L"CreateWindowEx - failed");
        return;
    }

    DWM_BLURBEHIND bb   = {0};
    HRGN           hRgn = CreateRectRgn(0, 0, -1, -1);
    bb.dwFlags          = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.hRgnBlur         = hRgn;
    bb.fEnable          = TRUE;
    DwmEnableBlurBehindWindow(WND, &bb);

    //                              ColorRef  Alpha,    By color or use the
    //                              previous alpha value
    // SetLayeredWindowAttributes(WND, 0x0,       128,   LWA_ALPHA
    // /*LWA_COLORKEY*/); SetLayeredWindowAttributes(WND, RGB(0,0,0), 128,
    // LWA_COLORKEY);

    DC = GetDC(WND);

    const int pixelAttribs[] = {WGL_DRAW_TO_WINDOW_ARB, GL_TRUE, WGL_SUPPORT_OPENGL_ARB, GL_TRUE, WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB, WGL_COLOR_BITS_ARB,
                                32, WGL_ALPHA_BITS_ARB, 8, WGL_DEPTH_BITS_ARB, 16,
                                // WGL_STENCIL_BITS_ARB, 8,
                                // WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
                                // WGL_SAMPLES_ARB, 4,
                                0};

    int        pixelFormatID;
    UINT       numFormats;
    const bool status = wglChoosePixelFormatARB(DC, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);

    if (status == false || numFormats == 0) {
        showMessage(L"wglChoosePixelFormatARB() failed.");
        return;
    }

    PIXELFORMATDESCRIPTOR PFD;
    DescribePixelFormat(DC, pixelFormatID, sizeof(PFD), &PFD);
    SetPixelFormat(DC, pixelFormatID, &PFD);

    const int major_min = 3, minor_min = 3;
    const int contextAttribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, major_min, WGL_CONTEXT_MINOR_VERSION_ARB, minor_min,
                                  WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                                  //		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
                                  0};
    RC                         = wglCreateContextAttribsARB(DC, 0, contextAttribs);
    if (RC == NULL) {
        showMessage(L"wglCreateContextAttribsARB() failed.");
        return;
    }

    // delete temporary context and window
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(fakeRC);
    ReleaseDC(fakeWND, fakeDC);
    DestroyWindow(fakeWND);
    if (!wglMakeCurrent(DC, RC)) {
        showMessage(L"wglMakeCurrent() failed.");
        return;
    }

    // Load extensions
    LOAD_GLEX(PFNGLBINDBUFFERPROC, glBindBuffer);
    LOAD_GLEX(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
    LOAD_GLEX(PFNGLGENBUFFERSPROC, glGenBuffers);
    LOAD_GLEX(PFNGLBUFFERDATAPROC, glBufferData);
    LOAD_GLEX(PFNGLATTACHSHADERPROC, glAttachShader);
    LOAD_GLEX(PFNGLCOMPILESHADERPROC, glCompileShader);
    LOAD_GLEX(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    LOAD_GLEX(PFNGLCREATESHADERPROC, glCreateShader);
    LOAD_GLEX(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    LOAD_GLEX(PFNGLDELETESHADERPROC, glDeleteShader);
    LOAD_GLEX(PFNGLDETACHSHADERPROC, glDetachShader);
    LOAD_GLEX(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    LOAD_GLEX(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation);
    LOAD_GLEX(PFNGLGETSHADERIVPROC, glGetShaderiv);
    LOAD_GLEX(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    LOAD_GLEX(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    LOAD_GLEX(PFNGLLINKPROGRAMPROC, glLinkProgram);
    LOAD_GLEX(PFNGLSHADERSOURCEPROC, glShaderSource);
    LOAD_GLEX(PFNGLUSEPROGRAMPROC, glUseProgram);
    LOAD_GLEX(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    LOAD_GLEX(PFNGLUNIFORM1FPROC, glUniform1f);
    LOAD_GLEX(PFNGLUNIFORM2FPROC, glUniform2f);
    LOAD_GLEX(PFNGLUNIFORM3FPROC, glUniform3f);
    LOAD_GLEX(PFNGLUNIFORM4FPROC, glUniform4f);
    LOAD_GLEX(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    LOAD_GLEX(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    LOAD_GLEX(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);
    LOAD_GLEX(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
    LOAD_GLEX(PFNGLBINDSAMPLERPROC, glBindSampler);
    LOAD_GLEX(PFNGLBLENDEQUATIONPROC, glBlendEquation);
    LOAD_GLEX(PFNGLACTIVETEXTUREPROC, glActiveTexture);
    LOAD_GLEX(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap);

    LOAD_GLEX(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
    LOAD_GLEX(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
    LOAD_GLEX(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers);
    LOAD_GLEX(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer);
    LOAD_GLEX(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers);
    LOAD_GLEX(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus);
    LOAD_GLEX(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage);
    LOAD_GLEX(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer);
    LOAD_GLEX(PFNGLFRAMEBUFFERTEXTUREPROC, glFramebufferTexture);
    LOAD_GLEX(PFNGLDRAWBUFFERSPROC, glDrawBuffers);

    SetWindowText(WND, reinterpret_cast<LPCWSTR>(glGetString(GL_VERSION)));
    ShowWindow(WND, globNCmdShow);

    if (!RegisterHotKey(WND, 1, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, 'G')) { showMessage(L"Unable to register the hotkey"); }

    DFT_CURSOR = LoadCursor(NULL, IDC_ARROW);
    CUR_CURSOR = DFT_CURSOR;

    // Compute the location of the application context, in the user data
    LPWSTR pProgramDataPath;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &pProgramDataPath))) {
        APP_PATH.clear();
        showMessage(L"Unable to get the user app data folder");
    } else {
        bsStringUtf16 path((char16_t*)pProgramDataPath);
        APP_PATH = path.toUtf8() + "/Yop";
        CoTaskMemFree(pProgramDataPath);
        // Create the directory if it does not yet exists
        if (!osalDirectoryExists(APP_PATH.toChar())) {
            if (CreateDirectory((wchar_t*)APP_PATH.toUtf16().toChar(), NULL)) {
                showMessage(L"Creation of the folder succeeded!");
            } else {
                showMessage(L"Error: Unable to create the folder");
            }
        }
    }
}

void
osalGetWindowSize(int& width, int& height)
{
    width  = globWindowWidth;
    height = globWindowHeight;
}

void
osalDestroyWindow()
{
    if (!UnregisterHotKey(WND, 1)) { showMessage(L"Unable to deregister the hotkey"); }

    wglMakeCurrent(NULL, NULL);
    if (RC) { wglDeleteContext(RC); }
    if (DC) { ReleaseDC(WND, DC); }
    if (WND) { DestroyWindow(WND); }
}

bool
osalIsMouseVisible()
{
    return (CUR_CURSOR == DFT_CURSOR);
}

void
osalSetMouseVisible(bool state)
{
    if (state == osalIsMouseVisible()) return;
    CUR_CURSOR = state ? DFT_CURSOR : NULL;
    SetCursor(CUR_CURSOR);
}

bsString
osalGetProgramDataPath()
{
    return APP_PATH;
}

bool
osalDirectoryExists(const bsString& path)
{
    DWORD attribs = GetFileAttributesW((wchar_t*)&path.toUtf16()[0]);
    if (attribs == INVALID_FILE_ATTRIBUTES) return false;
    return (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

FILE*
osalFileOpen(const bsString& path, const char* mode)
{
    return _wfopen((wchar_t*)&path.toUtf16()[0],
                   (wchar_t*)&bsString(mode).toUtf16()[0]);  // Utf-16 changes all APIs on Windows!
}

void
osalPushToClipboard(ClipboardType pushType, const bsStringUtf16& data)
{
    if (data.empty() || !OpenClipboard(NULL) || !EmptyClipboard()) return;  // In this order
    HANDLE clipHandle = GlobalAlloc(GMEM_MOVEABLE, (data.size() + 1) * sizeof(WCHAR));
    if (clipHandle) {
        wchar_t* ptr = (wchar_t*)GlobalLock(clipHandle);
        memcpy(ptr, &data[0], data.size() * sizeof(WCHAR));
        ptr[data.size()] = 0;
        GlobalUnlock(clipHandle);
        SetClipboardData(CF_UNICODETEXT, clipHandle);
    }
    CloseClipboard();
}

bsStringUtf16
osalReqFromClipboard(ClipboardType reqType)
{
    bsStringUtf16 result;
    // Open the clipboard
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return result;
    if (!OpenClipboard(NULL)) return result;
    // Get the data as Wchar (UTF-16 on windows)
    HANDLE clipHandle = GetClipboardData(CF_UNICODETEXT);
    if (!clipHandle) {
        CloseClipboard();
        return result;
    }
    wchar_t* ptr = (wchar_t*)GlobalLock(clipHandle);
    // We consider (erroneously) wchar as 16-bit truncated unicode
    while (*ptr) result.push_back((char16_t)(*ptr++));
    // Release the clipboard and return
    GlobalUnlock(clipHandle);
    CloseClipboard();
    return result;
}

void
osalSwapBuffer()
{
    SwapBuffers(DC);
}

void
osalHideWindow()
{
    ShowWindow(WND, SW_MINIMIZE);
}

void
osalShowWindow()
{
    ShowWindow(WND, SW_RESTORE);
}

static inline bool
isDisplayableKc(bsKeycode kc)
{
    return ((kc >= KC_A && kc <= KC_Num9) || (kc >= KC_LBracket && kc <= KC_Space) || (kc >= KC_Add && kc <= KC_Divide) ||
            (kc >= KC_Numpad0 && kc <= KC_Numpad9));
}

static bsOsHandler* osHandler = 0;

LRESULT CALLBACK
WindowProcedure(HWND hWnd, UINT messageType, WPARAM wParam, LPARAM lParam)
{
    bsKeycode     kc;
    bsKeyModState kms;

    // Sanity
    if (!osHandler) return DefWindowProc(hWnd, messageType, wParam, lParam);

    switch (messageType) {
        case WM_CHAR:
            if (bsIsUnicodeDisplayable((uint32_t)wParam)) { osHandler->eventChar((uint16_t)wParam); }
            break;

        case WM_KEYDOWN:
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            kc  = convertKeyCode(wParam, lParam);
            osHandler->eventKeyPressed(kc, kms);

            if (isDisplayableKc(kc)) {
                return DefWindowProc(hWnd, messageType, wParam,
                                     lParam);  // So that we have WM_CHAR events
            }

            break;

        case WM_KEYUP:
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            kc  = convertKeyCode(wParam, lParam);
            osHandler->eventKeyReleased(kc, kms);
            break;

        case WM_HOTKEY:
            osHandler->eventHotKeyPressed();
            break;

        case WM_KILLFOCUS:
            if (osHandler->isVisible()) {
                osalHideWindow();
                osHandler->notifyUnmapped();
            }
            break;

        case WM_SETFOCUS:
            if (!osHandler->isVisible()) { osHandler->notifyMapped(); }
            break;

        case WM_LBUTTONDOWN:
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            osHandler->eventButtonPressed(1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), kms);
            break;
        case WM_LBUTTONUP:
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            osHandler->eventButtonReleased(1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), kms);
            break;

        case WM_MBUTTONDOWN:
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            osHandler->eventButtonPressed(2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), kms);
            break;
        case WM_MBUTTONUP:
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            osHandler->eventButtonReleased(2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), kms);
            break;

        case WM_RBUTTONDOWN:
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            osHandler->eventButtonPressed(3, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), kms);
            break;
        case WM_RBUTTONUP:
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            osHandler->eventButtonReleased(3, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), kms);
            break;
        case WM_MOUSEWHEEL: {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);  // Coordinates for the wheel are screen ones...
            ScreenToClient(hWnd, &pt);
            kms = {HIWORD(GetKeyState(VK_SHIFT)) != 0, HIWORD(GetKeyState(VK_CONTROL)) != 0, HIWORD(GetKeyState(VK_MENU)) != 0,
                   HIWORD(GetKeyState(VK_LWIN)) || HIWORD(GetKeyState(VK_RWIN))};
            osHandler->eventWheelScrolled(pt.x, pt.y, (GET_WHEEL_DELTA_WPARAM(wParam) < 0) ? +1 : -1, kms);
        } break;
        case WM_MOUSEMOVE:
            osHandler->eventMouseMotion(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_CLOSE:
            osHandler->eventKeyPressed(KC_Escape, {false, false, false, false});
            PostQuitMessage(0);
            break;

        default:
            /*printf("IGNORED MT: 0x%x\n", messageType);*/
            return DefWindowProc(hWnd, messageType, wParam, lParam);
    }
    return 0;  // message handled
}

void
osalProcessInputs(bsOsHandler* handler)
{
    MSG msg;
    osHandler = handler;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { osHandler->eventKeyPressed(KC_Escape, {false, false, false, false}); }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Windows entry point
int APIENTRY
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Update global scope
    globHInstance = hInstance;
    globNCmdShow  = nCmdShow

        // @@@Incomplete Need to parse the unicode lpCmdLine to recreate our linux
        // parameters in UTF-8
        int argc   = 1;
    char*   argv[] = {"testGl"};

    return appBootstrap(argc, argv);
}

#endif  // _WIN32
