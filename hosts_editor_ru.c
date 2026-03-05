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

// Прототипы функций
void LogMessage(const wchar_t* message);
BOOL IsRunningAsAdmin();
void FlushDNSCache();
void LoadHostsFile();
void SaveHostsFile();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Логирование
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

// Проверка прав администратора
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

// Очистка DNS кэша
void FlushDNSCache() {
    LogMessage(L"Очистка DNS кэша...");
    system("ipconfig /flushdns");
    
    HINSTANCE hDnsApi = LoadLibrary(L"dnsapi.dll");
    if (hDnsApi) {
        typedef BOOL (WINAPI *DnsFlushResolverCache)();
        DnsFlushResolverCache pDnsFlushResolverCache = 
            (DnsFlushResolverCache)GetProcAddress(hDnsApi, "DnsFlushResolverCache");
        
        if (pDnsFlushResolverCache && pDnsFlushResolverCache()) {
            LogMessage(L"DNS кэш успешно очищен через DnsFlushResolverCache");
        }
        FreeLibrary(hDnsApi);
    }
    
    SetWindowTextW(hStatus, L"DNS кэш очищен");
}

// Загрузка файла hosts
void LoadHostsFile() {
    FILE* file = _wfopen(HOSTS_PATH, L"r, ccs=UTF-8");
    if (!file) {
        file = _wfopen(HOSTS_PATH, L"r");
    }
    
    if (file) {
        hostsContent[0] = L'\0';
        wchar_t buffer[1024];
        
        // Читаем файл построчно
        while (fgetws(buffer, sizeof(buffer)/sizeof(wchar_t), file)) {
            size_t len = wcslen(buffer);
            
            // Убираем символы новой строки в конце
            while (len > 0 && (buffer[len-1] == L'\n' || buffer[len-1] == L'\r')) {
                buffer[--len] = L'\0';
            }
            
            // Добавляем строку
            wcscat(buffer, L"\r\n");
            wcsncat(hostsContent, buffer, sizeof(hostsContent)/sizeof(wchar_t) - wcslen(hostsContent) - 1);
        }
        fclose(file);
        
        SetWindowTextW(hEdit, hostsContent);
        LogMessage(L"Файл hosts загружен");
        SetWindowTextW(hStatus, L"Файл загружен");
    } else {
        LogMessage(L"Ошибка загрузки файла hosts");
        SetWindowTextW(hStatus, L"Ошибка загрузки файла");
        LogMessage(L"Не удалось открыть файл hosts");
    }
}

// Сохранение файла hosts
void SaveHostsFile() {
    if (!IsRunningAsAdmin()) {
        LogMessage(L"Ошибка: Недостаточно прав для сохранения файла");
        LogMessage(L"Для сохранения файла hosts необходимы права администратора");
        LogMessage(L"Запустите программу от имени администратора");
        return;
    }
    
    GetWindowTextW(hEdit, hostsContent, sizeof(hostsContent)/sizeof(wchar_t));
    
    // Создаем резервную копию (перезаписываем старый бэкап)
    if (CopyFileW(HOSTS_PATH, HOSTS_BACKUP, FALSE)) {
        LogMessage(L"Создан бэкап файла: hosts.backup");
    } else {
        LogMessage(L"Ошибка создания бэкапа файла");
    }
    
    // Пробуем сохранить в UTF-8
    FILE* file = _wfopen(HOSTS_PATH, L"w, ccs=UTF-8");
    if (!file) {
        file = _wfopen(HOSTS_PATH, L"w");
    }
    
    if (file) {
        // Разбиваем текст на строки
        wchar_t* context = NULL;
        wchar_t* line = wcstok(hostsContent, L"\r\n", &context);
        
        while (line != NULL) {
            // Пишем строку в файл
            fputws(line, file);
            fputwc(L'\n', file);
            
            // Получаем следующую строку
            line = wcstok(NULL, L"\r\n", &context);
        }
        
        fclose(file);
        
        LogMessage(L"Файл hosts успешно сохранен");
        SetWindowTextW(hStatus, L"Файл сохранен");
        
        // Очищаем DNS кэш
        FlushDNSCache();
        
        LogMessage(L"Файл hosts успешно сохранен!");
        LogMessage(L"DNS кэш очищен.");
        LogMessage(L"Изменения применены без перезагрузки.");
    } else {
        LogMessage(L"Ошибка сохранения файла hosts");
        SetWindowTextW(hStatus, L"Ошибка сохранения");
        LogMessage(L"Не удалось сохранить файл hosts");
    }
}

// Создание главного окна
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
        L"Редактор файла hosts (Windows XP - 11)",
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

// Создание элементов управления
void CreateControls(HWND hwnd) {
    CreateWindowW(L"STATIC", L"Содержимое файла hosts:", 
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
    
    // Кнопки в ряд
    CreateWindowW(L"BUTTON", L"Загрузить",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 240, 100, 30, hwnd, (HMENU)ID_RELOAD, NULL, NULL);
    
    CreateWindowW(L"BUTTON", L"Сохранить",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        120, 240, 100, 30, hwnd, (HMENU)ID_SAVE, NULL, NULL);
    
    CreateWindowW(L"BUTTON", L"Проверить права",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        230, 240, 120, 30, hwnd, (HMENU)ID_CHECK_ADMIN, NULL, NULL);
    
    CreateWindowW(L"BUTTON", L"Очистить DNS",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        360, 240, 100, 30, hwnd, (HMENU)ID_FLUSHDNS, NULL, NULL);
    
    CreateWindowW(L"STATIC", L"Лог программы:", 
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 280, 200, 20, hwnd, NULL, NULL, NULL);
    
    hLog = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | 
        ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL | ES_READONLY,
        10, 300, 780, 200, hwnd, (HMENU)ID_LOG, NULL, NULL);
    
    SendMessage(hLog, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    hStatus = CreateWindowW(L"STATIC", L"Готов",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
        10, 510, 780, 20, hwnd, (HMENU)ID_STATUS, NULL, NULL);
}

// Обработчик сообщений окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            CreateControls(hwnd);
            LogMessage(L"Программа запущена");
            
            if (IsRunningAsAdmin()) {
                LogMessage(L"Программа запущена с правами администратора");
                LogMessage(L"Вы можете редактировать и сохранять файл hosts");
                SetWindowTextW(hStatus, L"Запущено с правами администратора");
            } else {
                LogMessage(L"ВНИМАНИЕ: Программа запущена без прав администратора");
                LogMessage(L"Сохранение файла hosts будет невозможно!");
                LogMessage(L"Для редактирования файла hosts:");
                LogMessage(L"1. Закройте программу");
                LogMessage(L"2. Щелкните правой кнопкой на ярлыке");
                LogMessage(L"3. Выберите 'Запуск от имени администратора'");
                SetWindowTextW(hStatus, L"Запущено без прав администратора");
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
                        LogMessage(L"Проверка прав: есть права администратора");
                        LogMessage(L"Вы можете редактировать и сохранять файл hosts");
                    } else {
                        LogMessage(L"Проверка прав: нет прав администратора");
                        LogMessage(L"Программа запущена БЕЗ прав администратора");
                        LogMessage(L"Для редактирования файла hosts:");
                        LogMessage(L"1. Закройте программу");
                        LogMessage(L"2. Щелкните правой кнопкой на ярлыке");
                        LogMessage(L"3. Выберите 'Запуск от имени администратора'");
                    }
                    break;
                    
                case ID_FLUSHDNS:
                    FlushDNSCache();
                    break;
            }
            return 0;
            
        case WM_DESTROY:
            LogMessage(L"Программа завершена");
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

// Точка входа
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