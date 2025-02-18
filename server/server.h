#pragma once

#define SERVER_CLASSNAME L"cmd_play server"
#define SERVER_TITLE L"cmd_play server"

inline HWND
Server_FindWindow(VOID)
{
    return FindWindowW(SERVER_CLASSNAME, SERVER_TITLE);
}

inline BOOL
Server_SendData(HWND hwndServer, const void *data_ptr, DWORD data_size)
{
    COPYDATASTRUCT cds = { 0xDEADFACE, data_size, (PVOID)data_ptr };
    return (BOOL)SendMessageW(hwndServer, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
}
