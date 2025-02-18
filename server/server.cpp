#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>
#include "server.h"

HINSTANCE g_hInst = NULL;
HWND g_hMainWnd = NULL;

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    g_hMainWnd = hwnd;
    return TRUE;
}

void OnDestroy(HWND hwnd)
{
    PostQuitMessage(0);
    g_hMainWnd = NULL;
}

BOOL OnCopyData(HWND hwnd, HWND hwndFrom, PCOPYDATASTRUCT pcds)
{
    if (pcds->dwData != 0xDEADFACE)
        return FALSE;

    DWORD cbData = pcds->cbData;
    PVOID lpData = pcds->lpData;
    return TRUE;
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_COPYDATA, OnCopyData);

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT
Server_ParseCmdLine(INT argc, LPWSTR *argv)
{
    return 0;
}

INT
Server_Main(HINSTANCE hInstance, INT argc, LPWSTR *argv, INT nCmdShow)
{
    if (INT ret = Server_ParseCmdLine(argc, argv))
        return ret;

    g_hInst = hInstance;
    InitCommonControls();

    WNDCLASSEXW wcx = { sizeof(wcx) };
    wcx.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = WindowProc;
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcx.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = SERVER_CLASSNAME;
    wcx.hIconSm = (HICON)LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    if (!RegisterClassEx(&wcx))
    {
        MessageBoxA(NULL, "RegisterClassEx failed", NULL, MB_ICONERROR);
        return -1;
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = 0;
    RECT rc = { 0, 0, 320, 200 };
    AdjustWindowRectEx(&rc, style, FALSE, exstyle);

    HWND hwnd = CreateWindowExW(exstyle, SERVER_CLASSNAME, SERVER_TITLE, style,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL);
    if (!hwnd)
    {
        MessageBoxA(NULL, "CreateWindowEx failed", NULL, MB_ICONERROR);
        return -2;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (INT)msg.wParam;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    INT argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    INT ret = Server_Main(hInstance, argc, argv, nCmdShow);
    LocalFree(argv);
    return ret;
}
