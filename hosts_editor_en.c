/*
================================================================================
Just lame Windows hosts file editor
For compilation, use this command:
windres resource.rc -o resource.o
gcc -o hosts_editor.exe hosts_editor.c resource.o -lcomctl32 -mwindows -municode

Copyright (C) 2026 playtester
================================================================================
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define ID_EDIT 1001
#define ID_SAVE 1002
#define ID_RELOAD 1003
#define ID_CHECK_ADMIN 1004
#define ID_LOG 1005
#define ID_STATUS 1006
#define ID_FLUSHDNS 1007
#define IDI_APP_ICON 100

#define HOSTS_PATH L"C:\\Windows\\System32\\drivers\\etc\\hosts"
#define HOSTS_BACKUP L"C:\\Windows\\System32\\drivers\\etc\\hosts.backup"

HWND hEdit, hLog, hStatus;
wchar_t hostsContent[65536];
wchar_t logBuffer[4096];

// Function prototypes
void LogMessage(const wchar_t* message);
BOOL IsRunningAsAdmin();
void FlushDNSCache();
void LoadHostsFile();
void SaveHostsFile();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Logging
void LogMessage(const wchar_t* message) {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    wchar_t timestamp[64];
    wcsftime(timestamp, sizeof(timestamp)/sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", timeinfo);
    
    wchar_t logEntry[512];
    _snwprintf(logEntry, sizeof(logEntry)/sizeof(wchar_t), L"[%s] %s\r\n", timestamp, message);
    
    int len = GetWindowTextLengthW(hLog);
    SendMessageW(hLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hLog, EM_REPLACESEL, 0, (LPARAM)logEntry);
    SendMessageW(hLog, WM_VSCROLL, SB_BOTTOM, 0);
}

// Check administrator privileges
BOOL IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD dwSize;
        
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    
    return isAdmin;
}

// Flush DNS cache
void FlushDNSCache() {
    LogMessage(L"Flushing DNS cache...");
    system("ipconfig /flushdns");
    
    HINSTANCE hDnsApi = LoadLibrary(L"dnsapi.dll");
    if (hDnsApi) {
        typedef BOOL (WINAPI *DnsFlushResolverCache)();
        DnsFlushResolverCache pDnsFlushResolverCache = 
            (DnsFlushResolverCache)GetProcAddress(hDnsApi, "DnsFlushResolverCache");
        
        if (pDnsFlushResolverCache && pDnsFlushResolverCache()) {
            LogMessage(L"DNS cache successfully flushed via DnsFlushResolverCache");
        }
        FreeLibrary(hDnsApi);
    }
    
    SetWindowTextW(hStatus, L"DNS cache flushed");
}

// Load hosts file
void LoadHostsFile() {
    FILE* file = _wfopen(HOSTS_PATH, L"r, ccs=UTF-8");
    if (!file) {
        file = _wfopen(HOSTS_PATH, L"r");
    }
    
    if (file) {
        hostsContent[0] = L'\0';
        wchar_t buffer[1024];
        
        // Read file line by line
        while (fgetws(buffer, sizeof(buffer)/sizeof(wchar_t), file)) {
            size_t len = wcslen(buffer);
            
            // Remove newline characters at the end
            while (len > 0 && (buffer[len-1] == L'\n' || buffer[len-1] == L'\r')) {
                buffer[--len] = L'\0';
            }
            
            // Add the line
            wcscat(buffer, L"\r\n");
            wcsncat(hostsContent, buffer, sizeof(hostsContent)/sizeof(wchar_t) - wcslen(hostsContent) - 1);
        }
        fclose(file);
        
        SetWindowTextW(hEdit, hostsContent);
        LogMessage(L"Hosts file loaded");
        SetWindowTextW(hStatus, L"File loaded");
    } else {
        LogMessage(L"Error loading hosts file");
        SetWindowTextW(hStatus, L"Error loading file");
        LogMessage(L"Could not open hosts file");
    }
}

// Save hosts file
void SaveHostsFile() {
    if (!IsRunningAsAdmin()) {
        LogMessage(L"Error: Insufficient permissions to save file");
        LogMessage(L"Administrator privileges are required to save the hosts file");
        LogMessage(L"Run the program as administrator");
        return;
    }
    
    GetWindowTextW(hEdit, hostsContent, sizeof(hostsContent)/sizeof(wchar_t));
    
    // Create backup (overwrite old backup)
    if (CopyFileW(HOSTS_PATH, HOSTS_BACKUP, FALSE)) {
        LogMessage(L"Backup created: hosts.backup");
    } else {
        LogMessage(L"Error creating backup file");
    }
    
    // Try to save in UTF-8
    FILE* file = _wfopen(HOSTS_PATH, L"w, ccs=UTF-8");
    if (!file) {
        file = _wfopen(HOSTS_PATH, L"w");
    }
    
    if (file) {
        // Split text into lines
        wchar_t* context = NULL;
        wchar_t* line = wcstok(hostsContent, L"\r\n", &context);
        
        while (line != NULL) {
            // Write line to file
            fputws(line, file);
            fputwc(L'\n', file);
            
            // Get next line
            line = wcstok(NULL, L"\r\n", &context);
        }
        
        fclose(file);
        
        LogMessage(L"Hosts file successfully saved");
        SetWindowTextW(hStatus, L"File saved");
        
        // Flush DNS cache
        FlushDNSCache();
        
        LogMessage(L"Hosts file successfully saved!");
        LogMessage(L"DNS cache flushed.");
        LogMessage(L"Changes applied without reboot.");
    } else {
        LogMessage(L"Error saving hosts file");
        SetWindowTextW(hStatus, L"Error saving");
        LogMessage(L"Could not save hosts file");
    }
}

// Create main window
HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"HostsEditorClass";
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    RegisterClassW(&wc);
    
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Hosts File Editor (Windows XP - 11)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (hwnd == NULL) {
        return NULL;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    return hwnd;
}

// Create controls
void CreateControls(HWND hwnd) {
    CreateWindowW(L"STATIC", L"Hosts file content:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 10, 200, 20, hwnd, NULL, NULL, NULL);
    
    hEdit = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | 
        ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
        10, 30, 780, 200, hwnd, (HMENU)ID_EDIT, NULL, NULL);
    
    HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");
    SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Buttons in a row
    CreateWindowW(L"BUTTON", L"Load",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 240, 100, 30, hwnd, (HMENU)ID_RELOAD, NULL, NULL);
    
    CreateWindowW(L"BUTTON", L"Save",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        120, 240, 100, 30, hwnd, (HMENU)ID_SAVE, NULL, NULL);
    
    CreateWindowW(L"BUTTON", L"Permissions",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        230, 240, 120, 30, hwnd, (HMENU)ID_CHECK_ADMIN, NULL, NULL);
    
    CreateWindowW(L"BUTTON", L"Clear DNS",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        360, 240, 100, 30, hwnd, (HMENU)ID_FLUSHDNS, NULL, NULL);
    
    CreateWindowW(L"STATIC", L"Program Log:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 280, 200, 20, hwnd, NULL, NULL, NULL);
    
    hLog = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | 
        ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL | ES_READONLY,
        10, 300, 780, 200, hwnd, (HMENU)ID_LOG, NULL, NULL);
    
    SendMessage(hLog, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    hStatus = CreateWindowW(L"STATIC", L"Ready",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
        10, 510, 780, 20, hwnd, (HMENU)ID_STATUS, NULL, NULL);
}

// Window message handler
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            CreateControls(hwnd);
            LogMessage(L"Program started");
            
            if (IsRunningAsAdmin()) {
                LogMessage(L"Program running with administrator privileges");
                LogMessage(L"You can edit and save the hosts file");
                SetWindowTextW(hStatus, L"Running as administrator");
            } else {
                LogMessage(L"WARNING: Program running without administrator privileges");
                LogMessage(L"Saving the hosts file will be impossible!");
                LogMessage(L"To edit the hosts file:");
                LogMessage(L"1. Close the program");
                LogMessage(L"2. Right-click on the shortcut");
                LogMessage(L"3. Select 'Run as administrator'");
                SetWindowTextW(hStatus, L"Running without administrator privileges");
            }
            
            LoadHostsFile();
            return 0;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_RELOAD:
                    LoadHostsFile();
                    break;
                    
                case ID_SAVE:
                    SaveHostsFile();
                    break;
                    
                case ID_CHECK_ADMIN:
                    if (IsRunningAsAdmin()) {
                        LogMessage(L"Permission check: has administrator privileges");
                        LogMessage(L"You can edit and save the hosts file");
                    } else {
                        LogMessage(L"Permission check: no administrator privileges");
                        LogMessage(L"Program running WITHOUT administrator privileges");
                        LogMessage(L"To edit the hosts file:");
                        LogMessage(L"1. Close the program");
                        LogMessage(L"2. Right-click on the shortcut");
                        LogMessage(L"3. Select 'Run as administrator'");
                    }
                    break;
                    
                case ID_FLUSHDNS:
                    FlushDNSCache();
                    break;
            }
            return 0;
            
        case WM_DESTROY:
            LogMessage(L"Program terminated");
            PostQuitMessage(0);
            return 0;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
            
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            
            MoveWindow(hEdit, 10, 30, width - 20, 200, TRUE);
            MoveWindow(hLog, 10, 300, width - 20, height - 340, TRUE);
            MoveWindow(hStatus, 10, height - 30, width - 20, 20, TRUE);
            
            return 0;
        }
    }
    
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// Entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPWSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    HWND hwnd = CreateMainWindow(hInstance, nCmdShow);
    if (hwnd == NULL) {
        return 1;
    }
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}