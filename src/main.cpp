#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <map>
#include <string>
#include <shlwapi.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_APP_ICON 101
#define ID_TRAY_EXIT 200
#define ID_APP_KOMOREBI 201
#define ID_APP_GOTIFY 202
#define ID_APP_PLEX 203

NOTIFYICONDATA nid = {};
HWND hwnd;
std::map<int, std::wstring> appMap = {
    {ID_APP_KOMOREBI, L"C:\\Program Files\\komorebi\\bin\\komorebi.exe"},
    {ID_APP_GOTIFY, L"D:\\gotify\\gotify.exe"},
    {ID_APP_PLEX, L"D:\\Plex\\PlexMediaServer\\Plex Media Server.exe"}
};

bool isProcessRunning(const wchar_t* exeName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    // Extract filename from path
    const wchar_t* exeFileName = exeName;
    const wchar_t* lastSlash = wcsrchr(exeName, L'\\');
    if (lastSlash) exeFileName = lastSlash + 1;

    bool found = false;
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeFileName) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return found;
}

bool terminateProcessByName(const wchar_t* exeName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    // Extract filename from path
    const wchar_t* exeFileName = exeName;
    const wchar_t* lastSlash = wcsrchr(exeName, L'\\');
    if (lastSlash) exeFileName = lastSlash + 1;

    bool terminated = false;
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeFileName) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    terminated = true;
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return terminated;
}

void toggleApp(const wchar_t* exeName) {
    if (isProcessRunning(exeName)) {
        terminateProcessByName(exeName);
    } else {
        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        // Get parent directory using PathRemoveFileSpec
        wchar_t dirBuffer[MAX_PATH];
        lstrcpyW(dirBuffer, exeName);
        PathRemoveFileSpecW(dirBuffer);

        PROCESS_INFORMATION pi;
        CreateProcessW(
            NULL,
            const_cast<LPWSTR>(exeName),
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW,
            NULL,
            dirBuffer,
            &si,
            &pi
        );
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

std::wstring getStatusLabel(const std::wstring& appName, const wchar_t* exeName) {
    bool running = isProcessRunning(exeName);
    return appName + (running ? L" (Running)" : L" (Idle)");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_TRAYICON) {
        if (lParam == WM_LBUTTONUP) {
            // Show app toggles menu on left click
            Sleep(150);
            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, ID_APP_KOMOREBI, getStatusLabel(L"Komorebi", appMap[ID_APP_KOMOREBI].c_str()).c_str());
            AppendMenu(menu, MF_STRING, ID_APP_GOTIFY, getStatusLabel(L"Gotify", appMap[ID_APP_GOTIFY].c_str()).c_str());
            AppendMenu(menu, MF_STRING, ID_APP_PLEX, getStatusLabel(L"Plex", appMap[ID_APP_PLEX].c_str()).c_str());
            // No Exit button on left click
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(menu);
        } else if (lParam == WM_RBUTTONUP) {
            // Show only Exit button on right click
            Sleep(100);
            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(menu);
        }
    } else if (message == WM_COMMAND) {
        int cmd = LOWORD(wParam);
        if (appMap.count(cmd)) {
            toggleApp(appMap[cmd].c_str());
            // Optionally, force menu to close and user to reopen for updated status
            // SetForegroundWindow(hWnd);
        } else if (cmd == ID_TRAY_EXIT) {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
        }
    } else if (message == WM_DESTROY) {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    const wchar_t CLASS_NAME[] = L"uTray";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ID_TRAY_APP_ICON));
    RegisterClass(&wc);

    hwnd = CreateWindowExW(0, CLASS_NAME, L"HiddenWindow", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_APP_ICON;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ID_TRAY_APP_ICON));
    lstrcpy(nid.szTip, TEXT("uTray"));

    Shell_NotifyIcon(NIM_ADD, &nid);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
