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

// Control IDs
#define ID_EDIT 1001
#define ID_SAVE 1002
#define ID_RELOAD 1003
#define ID_CHECK_ADMIN 1004
#define ID_LOG 1005
#define ID_STATUS 1006
#define ID_FLUSHDNS 1007
#define ID_LOGMSG 1008
#define ID_HELP 1009

// Icon
#define IDI_APP_ICON 100

// String IDs for multilingual support
#define IDS_HOSTS_CONTENT 2000
#define IDS_BTN_LOAD 2001
#define IDS_BTN_SAVE 2002
#define IDS_BTN_PERMISSIONS 2003
#define IDS_BTN_FLUSHDNS 2004
#define IDS_BTN_HELP 2005
#define IDS_LOG_PROGRAM 2006
#define IDS_STATUS_READY 2007
#define IDS_WINDOW_TITLE 2008
#define IDS_ADMIN_WARNING 2009
#define IDS_ADMIN_RUNNING 2010
#define IDS_NO_ADMIN 2011
#define IDS_PERMISSION_CHECK 2012
#define IDS_HAS_ADMIN 2013
#define IDS_NO_ADMIN_PERM 2014
#define IDS_RUN_AS_ADMIN 2015
#define IDS_LOAD_ERROR 2030
#define IDS_SAVE_ERROR 2031
#define IDS_MEMORY_ERROR 2032
#define IDS_FILE_EMPTY 2033
#define IDS_FILE_LOADED 2034
#define IDS_FILE_SAVED 2035
#define IDS_BACKUP_CREATED 2036
#define IDS_BACKUP_ERROR 2037
#define IDS_DNS_FLUSHING 2038
#define IDS_DNS_SUCCESS 2039
#define IDS_DNS_ERROR 2040
#define IDS_ENCODING_UTF8 2041
#define IDS_ENCODING_ANSI 2042
#define IDS_ENCODING_UNKNOWN 2043
#define IDS_SAVING_UTF8 2044
#define IDS_SAVING_ANSI 2045
#define IDS_CHANGES_APPLIED 2046
#define IDS_CAN_EDIT 2047
#define IDS_SYSTEM_LANG 2048
#define IDS_PROGRAM_STARTED 2049
#define IDS_PROGRAM_TERMINATED 2050
#define IDS_CLOSE_STEP1 2051
#define IDS_CLOSE_STEP2 2052
#define IDS_CLOSE_STEP3 2053

#define HOSTS_PATH L"C:\\Windows\\System32\\drivers\\etc\\hosts"
#define HOSTS_BACKUP L"C:\\Windows\\System32\\drivers\\etc\\hosts.backup"

UINT g_fileCodePage = CP_UTF8;
BOOL g_bIsRussian = FALSE;  // TRUE for Russian UI, FALSE for English

HWND hEdit, hLog, hStatus;
wchar_t* hostsContent = NULL;
size_t hostsContentSize = 0;
wchar_t logBuffer[4096];

// Function prototypes
void LogMessage(const wchar_t* message);
BOOL IsRunningAsAdmin();
void FlushDNSCache();
void LoadHostsFile();
void SaveHostsFile();
void ShowHelp();
BOOL IsSystemRussian();
const wchar_t* GetString(int stringId);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Get system language name
const wchar_t* GetSystemLanguageName() {
    static wchar_t langName[128];
    LANGID langId = GetSystemDefaultLangID();

    if (GetLocaleInfoW(langId, LOCALE_SENGLANGUAGE, langName, 128)) {
        return langName;
    }

    return L"Unknown";
}

// Check system language (returns TRUE for Russian UI, FALSE for English)
BOOL IsSystemRussian() {
    LANGID langId = GetSystemDefaultLangID();

    // Former USSR republics & Russia
    switch (langId) {
    case 0x0419: // Russian
    case 0x0422: // Ukrainian
    case 0x0423: // Belarusian
    case 0x042B: // Armenian
    case 0x042C: // Azerbaijani
    case 0x0437: // Georgian
    case 0x0428: // Tajik
    case 0x0843: // Uzbek
    case 0x043F: // Kazakh
    case 0x0440: // Kyrgyz
    case 0x0442: // Turkmen
        return TRUE;
    }

    return FALSE;
}

// Get localized string based on system language
const wchar_t* GetString(int stringId) {
    if (g_bIsRussian) {
        switch (stringId) {
            case IDS_HOSTS_CONTENT: return L"Содержимое файла hosts:";
            case IDS_BTN_LOAD: return L"Загрузить";
            case IDS_BTN_SAVE: return L"Сохранить";
            case IDS_BTN_PERMISSIONS: return L"Проверить права";
            case IDS_BTN_FLUSHDNS: return L"Очистить DNS";
            case IDS_BTN_HELP: return L"Справка";
            case IDS_LOG_PROGRAM: return L"Лог программы:";
            case IDS_STATUS_READY: return L"Готов";
            case IDS_WINDOW_TITLE: return L"Редактор файла hosts (Windows XP - 11)";
            case IDS_ADMIN_WARNING: return L"ВНИМАНИЕ: Программа запущена без прав администратора";
            case IDS_ADMIN_RUNNING: return L"Программа запущена с правами администратора";
            case IDS_NO_ADMIN: return L"Сохранение файла hosts будет невозможно!";
            case IDS_PERMISSION_CHECK: return L"Проверка прав:";
            case IDS_HAS_ADMIN: return L"Есть права администратора";
            case IDS_NO_ADMIN_PERM: return L"Нет прав администратора";
            case IDS_RUN_AS_ADMIN: return L"Запустите программу от имени администратора";
            case IDS_CLOSE_STEP1: return L"1. Закройте программу";
            case IDS_CLOSE_STEP2: return L"2. Щелкните правой кнопкой на ярлыке";
            case IDS_CLOSE_STEP3: return L"3. Выберите 'Запуск от имени администратора'";
            case IDS_LOAD_ERROR: return L"Ошибка загрузки файла hosts";
            case IDS_SAVE_ERROR: return L"Ошибка сохранения файла hosts";
            case IDS_MEMORY_ERROR: return L"Ошибка выделения памяти";
            case IDS_FILE_EMPTY: return L"Файл hosts пуст";
            case IDS_FILE_LOADED: return L"Файл hosts загружен";
            case IDS_FILE_SAVED: return L"Файл hosts успешно сохранен";
            case IDS_BACKUP_CREATED: return L"Создана резервная копия hosts.backup";
            case IDS_BACKUP_ERROR: return L"Ошибка создания резервной копии";
            case IDS_DNS_FLUSHING: return L"Очистка DNS кэша...";
            case IDS_DNS_SUCCESS: return L"DNS кэш успешно очищен";
            case IDS_DNS_ERROR: return L"Ошибка при очистке DNS кэша";
            case IDS_ENCODING_UTF8: return L"Файл загружен (кодировка: UTF-8)";
            case IDS_ENCODING_ANSI: return L"Файл загружен (кодировка: ANSI)";
            case IDS_ENCODING_UNKNOWN: return L"Ошибка: не удалось определить кодировку";
            case IDS_SAVING_UTF8: return L"Сохранение в кодировке UTF-8";
            case IDS_SAVING_ANSI: return L"Сохранение в кодировке ANSI";
            case IDS_CHANGES_APPLIED: return L"Изменения применены";
            case IDS_PROGRAM_STARTED: return L"Программа запущена";
            case IDS_PROGRAM_TERMINATED: return L"Программа завершена";
	    case IDS_CAN_EDIT: return L"Вы можете редактировать и сохранять файл hosts";
	    case IDS_SYSTEM_LANG: return L"Язык системы: %s (используется %s интерфейс)";
            default: return L"";
        }
    } else {
        switch (stringId) {
            case IDS_HOSTS_CONTENT: return L"Hosts file content:";
            case IDS_BTN_LOAD: return L"Load";
            case IDS_BTN_SAVE: return L"Save";
            case IDS_BTN_PERMISSIONS: return L"Permissions";
            case IDS_BTN_FLUSHDNS: return L"Flush DNS";
            case IDS_BTN_HELP: return L"Help";
            case IDS_LOG_PROGRAM: return L"Program log:";
            case IDS_STATUS_READY: return L"Ready";
            case IDS_WINDOW_TITLE: return L"Hosts File Editor (Windows XP - 11)";
            case IDS_ADMIN_WARNING: return L"WARNING: Program running without administrator privileges";
            case IDS_ADMIN_RUNNING: return L"Program running with administrator privileges";
            case IDS_NO_ADMIN: return L"Saving the hosts file will be impossible!";
            case IDS_PERMISSION_CHECK: return L"Permission check:";
            case IDS_HAS_ADMIN: return L"Has administrator privileges";
            case IDS_NO_ADMIN_PERM: return L"No administrator privileges";
            case IDS_RUN_AS_ADMIN: return L"Run the program as administrator";
            case IDS_CLOSE_STEP1: return L"1. Close the program";
            case IDS_CLOSE_STEP2: return L"2. Right-click on the shortcut";
            case IDS_CLOSE_STEP3: return L"3. Select 'Run as administrator'";
            case IDS_LOAD_ERROR: return L"Error loading hosts file";
            case IDS_SAVE_ERROR: return L"Error saving hosts file";
            case IDS_MEMORY_ERROR: return L"Memory allocation error";
            case IDS_FILE_EMPTY: return L"Hosts file is empty";
            case IDS_FILE_LOADED: return L"Hosts file loaded";
            case IDS_FILE_SAVED: return L"Hosts file successfully saved";
            case IDS_BACKUP_CREATED: return L"Backup created: hosts.backup";
            case IDS_BACKUP_ERROR: return L"Error creating backup";
            case IDS_DNS_FLUSHING: return L"Flushing DNS cache...";
            case IDS_DNS_SUCCESS: return L"DNS cache successfully flushed";
            case IDS_DNS_ERROR: return L"Error flushing DNS cache";
            case IDS_ENCODING_UTF8: return L"File loaded (encoding: UTF-8)";
            case IDS_ENCODING_ANSI: return L"File loaded (encoding: ANSI)";
            case IDS_ENCODING_UNKNOWN: return L"Error: could not determine file encoding";
            case IDS_SAVING_UTF8: return L"Saving in UTF-8 encoding";
            case IDS_SAVING_ANSI: return L"Saving in ANSI encoding";
            case IDS_CHANGES_APPLIED: return L"Changes applied";
            case IDS_PROGRAM_STARTED: return L"Program started";
            case IDS_PROGRAM_TERMINATED: return L"Program terminated";
	    case IDS_CAN_EDIT: return L"You can edit and save the hosts file";
	    case IDS_SYSTEM_LANG: return L"System language: %s (using %s UI)";
            default: return L"";
        }
    }
    return L"";
}

// Logging with timestamp
void LogMessage(const wchar_t* message) {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    wchar_t timestamp[64];
    wcsftime(timestamp, sizeof(timestamp) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", timeinfo);

    wchar_t logEntry[512];
    _snwprintf(logEntry, sizeof(logEntry) / sizeof(wchar_t), L"[%s] %s\r\n", timestamp, message);

    int len = GetWindowTextLengthW(hLog);
    SendMessageW(hLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(hLog, EM_REPLACESEL, 0, (LPARAM)logEntry);
    SendMessageW(hLog, WM_VSCROLL, SB_BOTTOM, 0);
}

// Check administrator privileges (works on Windows XP - 11)
BOOL IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID pAdminSid = NULL;

    // Create a SID for the Administrators group
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0, &pAdminSid)) {

        // Check if this SID is enabled in the token
        if (!CheckTokenMembership(NULL, pAdminSid, &isAdmin)) {
            isAdmin = FALSE;
        }

        FreeSid(pAdminSid);
    }

    return isAdmin;
}

// Flush DNS cache with fallback mechanism
void FlushDNSCache() {
    LogMessage(GetString(IDS_DNS_FLUSHING));

    BOOL dnsFlushed = FALSE;

    // First try via API
    HINSTANCE hDnsApi = LoadLibrary(L"dnsapi.dll");
    if (hDnsApi) {
        typedef BOOL(WINAPI * DnsFlushResolverCacheFunc)();
        DnsFlushResolverCacheFunc pDnsFlushResolverCache =
            (DnsFlushResolverCacheFunc)GetProcAddress(hDnsApi, "DnsFlushResolverCache");

        if (pDnsFlushResolverCache) {
            dnsFlushed = pDnsFlushResolverCache();
            if (dnsFlushed) {
                LogMessage(GetString(IDS_DNS_SUCCESS));
            }
        }
        FreeLibrary(hDnsApi);
    }

    // If API didn't work, try via ipconfig
    if (!dnsFlushed) {
        int result = system("ipconfig /flushdns");
        if (result == 0) {
            LogMessage(GetString(IDS_DNS_SUCCESS));
            dnsFlushed = TRUE;
        } else {
            LogMessage(GetString(IDS_DNS_ERROR));
        }
    }

    if (dnsFlushed) {
        SetWindowTextW(hStatus, GetString(IDS_DNS_SUCCESS));
    } else {
        SetWindowTextW(hStatus, GetString(IDS_DNS_ERROR));
    }
}

// Check for BOM (Byte Order Mark)
BOOL HasUTF8BOM(const char* buffer, size_t size) {
    if (size >= 3) {
        return (buffer[0] == '\xEF' && buffer[1] == '\xBB' && buffer[2] == '\xBF');
    }
    return FALSE;
}

// Check for valid UTF-8 sequence
BOOL IsValidUTF8(const char* buffer, size_t size) {
    int bytesRemaining = size;
    const unsigned char* ptr = (const unsigned char*)buffer;

    while (bytesRemaining > 0) {
        if (*ptr <= 0x7F) {
            // ASCII (0-127) - 1 byte
            ptr += 1;
            bytesRemaining -= 1;
        } else if (*ptr >= 0xC2 && *ptr <= 0xDF) {
            // 2-byte UTF-8 character (first bytes C2-DF)
            if (bytesRemaining < 2) return FALSE;
            if (ptr[1] < 0x80 || ptr[1] > 0xBF) return FALSE;
            ptr += 2;
            bytesRemaining -= 2;
        } else if (*ptr >= 0xE0 && *ptr <= 0xEF) {
            // 3-byte UTF-8 character (E0-EF)
            if (bytesRemaining < 3) return FALSE;
            if (*ptr == 0xE0 && (ptr[1] < 0xA0 || ptr[1] > 0xBF)) return FALSE;
            if (*ptr == 0xED && (ptr[1] < 0x80 || ptr[1] > 0x9F)) return FALSE;
            if (ptr[1] < 0x80 || ptr[1] > 0xBF) return FALSE;
            if (ptr[2] < 0x80 || ptr[2] > 0xBF) return FALSE;
            ptr += 3;
            bytesRemaining -= 3;
        } else if (*ptr >= 0xF0 && *ptr <= 0xF4) {
            // 4-byte UTF-8 character (F0-F4)
            if (bytesRemaining < 4) return FALSE;
            if (*ptr == 0xF0 && (ptr[1] < 0x90 || ptr[1] > 0xBF)) return FALSE;
            if (*ptr == 0xF4 && (ptr[1] < 0x80 || ptr[1] > 0x8F)) return FALSE;
            if (ptr[1] < 0x80 || ptr[1] > 0xBF) return FALSE;
            if (ptr[2] < 0x80 || ptr[2] > 0xBF) return FALSE;
            if (ptr[3] < 0x80 || ptr[3] > 0xBF) return FALSE;
            ptr += 4;
            bytesRemaining -= 4;
        } else {
            // Invalid byte for UTF-8
            return FALSE;
        }
    }
    return TRUE;
}

// Detect file encoding (UTF-8 or ANSI)
UINT DetectEncoding(const char* buffer, size_t size) {
    // First check for BOM
    if (HasUTF8BOM(buffer, size)) {
        return CP_UTF8;
    }

    // Then check for valid UTF-8
    if (IsValidUTF8(buffer, size)) {
        // Check if there are any multibyte characters at all
        int hasHighBits = 0;
        for (size_t i = 0; i < size; i++) {
            if (buffer[i] & 0x80) { // byte with high bit set
                hasHighBits++;
                if (hasHighBits > 5) break; // enough to be confident
            }
        }

        // If there are multibyte characters and valid UTF-8 - it's UTF-8
        if (hasHighBits > 0) {
            return CP_UTF8;
        }
    }

    // In all other cases - ANSI
    return CP_ACP;
}

// Load hosts file with encoding detection
void LoadHostsFile() {
    FILE* file = _wfopen(HOSTS_PATH, L"rb");
    if (!file) {
        LogMessage(GetString(IDS_LOAD_ERROR));
        SetWindowTextW(hStatus, GetString(IDS_LOAD_ERROR));
        return;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        fclose(file);
        LogMessage(GetString(IDS_FILE_EMPTY));
        SetWindowTextW(hEdit, L"");
        return;
    }

    // Read file
    char* tempBuffer = (char*)malloc(fileSize + 2);
    if (!tempBuffer) {
        fclose(file);
        LogMessage(GetString(IDS_MEMORY_ERROR));
        return;
    }

    size_t bytesRead = fread(tempBuffer, 1, fileSize, file);
    fclose(file);
    tempBuffer[bytesRead] = '\0';
    tempBuffer[bytesRead + 1] = '\0';

    // Free old buffer
    if (hostsContent) {
        free(hostsContent);
        hostsContent = NULL;
    }

    // Detect file encoding
    UINT detectedCodePage = DetectEncoding(tempBuffer, bytesRead);

    // Convert to UTF-16
    int wideSize = MultiByteToWideChar(detectedCodePage, 0, tempBuffer, -1, NULL, 0);

    if (wideSize > 0) {
        hostsContent = (wchar_t*)malloc(wideSize * sizeof(wchar_t));
        if (hostsContent) {
            MultiByteToWideChar(detectedCodePage, 0, tempBuffer, -1, hostsContent, wideSize);
            hostsContentSize = wideSize;
            g_fileCodePage = detectedCodePage;

            if (detectedCodePage == CP_UTF8) {
                LogMessage(GetString(IDS_ENCODING_UTF8));
            } else {
                LogMessage(GetString(IDS_ENCODING_ANSI));
            }
        } else {
            LogMessage(GetString(IDS_MEMORY_ERROR));
            g_fileCodePage = CP_UTF8;
        }
    } else {
        LogMessage(GetString(IDS_ENCODING_UNKNOWN));
        g_fileCodePage = CP_UTF8;
    }

    free(tempBuffer);

    if (hostsContent) {
        SetWindowTextW(hEdit, hostsContent);
        SetWindowTextW(hStatus, GetString(IDS_FILE_LOADED));
    } else {
        SetWindowTextW(hEdit, L"");
        SetWindowTextW(hStatus, GetString(IDS_LOAD_ERROR));
    }
}

// Save hosts file preserving original encoding
void SaveHostsFile() {
    if (!IsRunningAsAdmin()) {
        LogMessage(GetString(IDS_SAVE_ERROR));
        LogMessage(GetString(IDS_NO_ADMIN));
        LogMessage(GetString(IDS_RUN_AS_ADMIN));
        return;
    }

    // Get text from editor
    int textLength = GetWindowTextLengthW(hEdit);
    wchar_t* editBuffer = (wchar_t*)malloc((textLength + 100) * sizeof(wchar_t));

    if (!editBuffer) {
        LogMessage(GetString(IDS_MEMORY_ERROR));
        return;
    }

    GetWindowTextW(hEdit, editBuffer, textLength + 10);

    // Create backup
    if (CopyFileW(HOSTS_PATH, HOSTS_BACKUP, FALSE)) {
        LogMessage(GetString(IDS_BACKUP_CREATED));
    } else {
        LogMessage(GetString(IDS_BACKUP_ERROR));
    }

    // Save file
    FILE* file = _wfopen(HOSTS_PATH, L"wb");
    if (!file) {
        LogMessage(GetString(IDS_SAVE_ERROR));
        free(editBuffer);
        return;
    }

    // Create copy of string for parsing
    wchar_t* lineCopy = _wcsdup(editBuffer);
    if (!lineCopy) {
        LogMessage(GetString(IDS_MEMORY_ERROR));
        fclose(file);
        free(editBuffer);
        return;
    }

    // Use saved encoding
    UINT codePage = g_fileCodePage;

    // Log which encoding we're saving in
    if (codePage == CP_UTF8) {
        LogMessage(GetString(IDS_SAVING_UTF8));
    } else {
        LogMessage(GetString(IDS_SAVING_ANSI));
    }

    // Parse and save
    wchar_t* context;
    wchar_t* line = wcstok(lineCopy, L"\n", &context);
    int lineCount = 0;

    while (line != NULL) {
        // Remove \r at end of line if present
        size_t len = wcslen(line);
        if (len > 0 && line[len - 1] == L'\r') {
            line[len - 1] = L'\0';
        }

        // Convert to desired encoding
        int mbSize = WideCharToMultiByte(codePage, 0, line, -1, NULL, 0, NULL, NULL);
        char* mbLine = (char*)malloc(mbSize + 2);

        if (mbLine) {
            WideCharToMultiByte(codePage, 0, line, -1, mbLine, mbSize, NULL, NULL);
            fwrite(mbLine, 1, mbSize - 1, file);  // -1 to remove null
            fwrite("\r\n", 1, 2, file);  // add line endings
            free(mbLine);
            lineCount++;
        }

        line = wcstok(NULL, L"\n", &context);
    }

    fclose(file);
    free(lineCopy);

    // Update global buffer
    if (hostsContent) free(hostsContent);
    hostsContent = _wcsdup(editBuffer);
    hostsContentSize = textLength + 1;
    free(editBuffer);

    LogMessage(GetString(IDS_FILE_SAVED));
    SetWindowTextW(hStatus, GetString(IDS_FILE_SAVED));

    // Flush DNS cache
    FlushDNSCache();

    LogMessage(GetString(IDS_CHANGES_APPLIED));
}

// Display help information
void ShowHelp() {
    LogMessage(L"");
    if (g_bIsRussian) {
        LogMessage(L"================= Редактор файла hosts v1.01 ====================");
        LogMessage(L"Это не требующая установки утилита для просмотра и редактирования");
        LogMessage(L"системного файла hosts с определением прав администратора,");
        LogMessage(L"автоматическим созданием резервных копий и сбросом DNS-кеша.");
        LogMessage(L"Работает на всех версиях Windows от XP до 11.");
        LogMessage(L"");
        LogMessage(L"Если этот инструмент Вам полезен и хотите поддержать его автора,");
        LogMessage(L"Вы можете отправить донат TRC-20 USDT/TRX:");
    } else {
        LogMessage(L"=========== Windows hosts file editor v1.01 ================");
        LogMessage(L"This is a portable utility for viewing and editing the");
        LogMessage(L"system hosts file with administrator privilege detection,");
        LogMessage(L"automatic backup creation, and DNS cache flushing.");
        LogMessage(L"Works on all Windows versions from XP to 11.");
        LogMessage(L"");
        LogMessage(L"If you find this tool useful and want to support its author,");
        LogMessage(L"you can send a donation TRC-20 USDT/TRX:");
    }
    LogMessage(L"TTjnMhCcus7cibpAyx7PqaiQPuu4L6NV1a");
    LogMessage(L"");
    LogMessage(L"© 2026 playtester");
}

// Create main window
HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"HostsEditorClass";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
                    0,
                    CLASS_NAME,
                    GetString(IDS_WINDOW_TITLE),
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

// Create UI controls
void CreateControls(HWND hwnd) {
    CreateWindowW(L"STATIC", GetString(IDS_HOSTS_CONTENT),
                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                  10, 10, 200, 20, hwnd, NULL, NULL, NULL);

    hEdit = CreateWindowW(L"EDIT", L"",
                          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                          ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
                          10, 30, 780, 200, hwnd, (HMENU)ID_EDIT, NULL, NULL);

    HFONT hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Buttons row (5 buttons)
    CreateWindowW(L"BUTTON", GetString(IDS_BTN_LOAD),
                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  10, 240, 100, 30, hwnd, (HMENU)ID_RELOAD, NULL, NULL);

    CreateWindowW(L"BUTTON", GetString(IDS_BTN_SAVE),
                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  120, 240, 100, 30, hwnd, (HMENU)ID_SAVE, NULL, NULL);

    CreateWindowW(L"BUTTON", GetString(IDS_BTN_PERMISSIONS),
                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  230, 240, 120, 30, hwnd, (HMENU)ID_CHECK_ADMIN, NULL, NULL);

    CreateWindowW(L"BUTTON", GetString(IDS_BTN_FLUSHDNS),
                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  360, 240, 100, 30, hwnd, (HMENU)ID_FLUSHDNS, NULL, NULL);


    CreateWindowW(L"BUTTON", GetString(IDS_BTN_HELP),
                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  470, 240, 100, 30, hwnd, (HMENU)ID_HELP, NULL, NULL);

    CreateWindowW(L"STATIC", GetString(IDS_LOG_PROGRAM),
                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                  10, 280, 200, 20, hwnd, (HMENU)ID_LOGMSG, NULL, NULL);

    hLog = CreateWindowW(L"EDIT", L"",
                         WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                         ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL | ES_READONLY,
                         10, 300, 780, 200, hwnd, (HMENU)ID_LOG, NULL, NULL);

    SendMessage(hLog, WM_SETFONT, (WPARAM)hFont, TRUE);

    hStatus = CreateWindowW(L"STATIC", GetString(IDS_STATUS_READY),
                            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_SUNKEN,
                            10, 510, 780, 20, hwnd, (HMENU)ID_STATUS, NULL, NULL);
}

// Window message handler
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        // Detect system language at startup
        g_bIsRussian = IsSystemRussian();

        CreateControls(hwnd);

        // Log actual system language
        wchar_t langMsg[256];
        const wchar_t* sysLang = GetSystemLanguageName();
        const wchar_t* uiLang = g_bIsRussian ? L"Russian" : L"English";

        const wchar_t* formatStr = GetString(IDS_SYSTEM_LANG);
        _snwprintf(langMsg, 256, formatStr, sysLang, uiLang);
        LogMessage(langMsg);

        LogMessage(GetString(IDS_PROGRAM_STARTED));

        if (IsRunningAsAdmin()) {
            LogMessage(GetString(IDS_ADMIN_RUNNING));
            LogMessage(GetString(IDS_CAN_EDIT));
            SetWindowTextW(hStatus, GetString(IDS_ADMIN_RUNNING));
        } else {
            LogMessage(GetString(IDS_ADMIN_WARNING));
            LogMessage(GetString(IDS_NO_ADMIN));
            LogMessage(GetString(IDS_CLOSE_STEP1));
            LogMessage(GetString(IDS_CLOSE_STEP2));
            LogMessage(GetString(IDS_CLOSE_STEP3));
            SetWindowTextW(hStatus, GetString(IDS_ADMIN_WARNING));
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
                LogMessage(GetString(IDS_PERMISSION_CHECK));
                LogMessage(GetString(IDS_HAS_ADMIN));
            } else {
                LogMessage(GetString(IDS_PERMISSION_CHECK));
                LogMessage(GetString(IDS_NO_ADMIN_PERM));
                LogMessage(GetString(IDS_RUN_AS_ADMIN));
            }
            break;

        case ID_FLUSHDNS:
            FlushDNSCache();
            break;

        case ID_HELP:
            ShowHelp();
            break;
        }
        return 0;

    case WM_DESTROY:
        LogMessage(GetString(IDS_PROGRAM_TERMINATED));

        // Free dynamic memory
        if (hostsContent) {
            free(hostsContent);
            hostsContent = NULL;
        }

        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        // Hosts editor grows downward
        MoveWindow(hEdit, 10, 30, width - 20, height - 340, TRUE);

        // Buttons under hosts (update positions of all 5 buttons)
        int buttonY = 30 + (height - 340) + 10;
        MoveWindow(GetDlgItem(hwnd, ID_RELOAD), 10, buttonY, 100, 30, TRUE);
        MoveWindow(GetDlgItem(hwnd, ID_SAVE), 120, buttonY, 100, 30, TRUE);
        MoveWindow(GetDlgItem(hwnd, ID_CHECK_ADMIN), 230, buttonY, 120, 30, TRUE);
        MoveWindow(GetDlgItem(hwnd, ID_FLUSHDNS), 360, buttonY, 100, 30, TRUE);
        MoveWindow(GetDlgItem(hwnd, ID_HELP), 470, buttonY, 100, 30, TRUE);

        int logLabelY = buttonY + 40;
        MoveWindow(GetDlgItem(hwnd, ID_LOGMSG), 10, logLabelY, 200, 20, TRUE);
        MoveWindow(hLog, 10, logLabelY + 20, width - 20, 200, TRUE);
        MoveWindow(hStatus, 10, height - 30, width - 20, 20, TRUE);
        RedrawWindow(hLog, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

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