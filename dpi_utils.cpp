// dpi_utils.cpp

#include <windows.h>
#include <math.h>

// На старых SDK может не быть константы — определяем безопасно
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE)-4)
#endif

// MDT_EFFECTIVE_DPI обычно = 0
#ifndef MDT_EFFECTIVE_DPI
#define MDT_EFFECTIVE_DPI 0
#endif

// статические переменные ссылок на DLL
static HMODULE g_hUser32 = NULL;
static HMODULE g_hShcore = NULL;

static bool    g_dpiInitialized = false; // вызывалась ли уже функция InitDpi()

// указатели на функции внутри DLL (runtime lookup)
static BOOL(WINAPI* g_pSetProcessDpiAwarenessContext)(HANDLE) = NULL; // user32 (Win10+)
static HRESULT(WINAPI* g_pSetProcessDpiAwareness)(int) = NULL;          // shcore  (8.1+)
static BOOL(WINAPI* g_pSetProcessDPIAware)(void) = NULL;             // user32 (Vista+)

static UINT(WINAPI* g_pGetDpiForWindow)(HWND) = NULL;                // user32 (Win10+)
static HRESULT(WINAPI* g_pGetDpiForMonitor)(HMONITOR, int, UINT*, UINT*) = NULL; // shcore (8.1+)


// Инициализация (загружает модули и выставляет awareness)
// Вызывать один раз в WinMain ДО создания окон.
void InitDpi(void)
{
    if (g_dpiInitialized) 
        return;

    g_dpiInitialized = true;

    // Загружаем user32.dll (сохраняем навсегда)
    g_hUser32 = LoadLibraryW(L"user32.dll");
    if (g_hUser32)
    {
        g_pSetProcessDpiAwarenessContext =
            (BOOL(WINAPI*)(HANDLE)) GetProcAddress(g_hUser32, "SetProcessDpiAwarenessContext");

        g_pSetProcessDPIAware =
            (BOOL(WINAPI*)(void)) GetProcAddress(g_hUser32, "SetProcessDPIAware");

        g_pGetDpiForWindow =
            (UINT(WINAPI*)(HWND)) GetProcAddress(g_hUser32, "GetDpiForWindow");
    }

    // Загружаем shcore.dll, если доступна (Windows 8.1+)
    g_hShcore = LoadLibraryW(L"shcore.dll");
    if (g_hShcore)
    {
        g_pSetProcessDpiAwareness =
            (HRESULT(WINAPI*)(int)) GetProcAddress(g_hShcore, "SetProcessDpiAwareness");

        g_pGetDpiForMonitor =
            (HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*)) GetProcAddress(g_hShcore, "GetDpiForMonitor");
    }

    // Сначала пробуем самый современный API (Win10 Per-Monitor V2)
    if (g_pSetProcessDpiAwarenessContext)
    {
        // Попытка установить Per-Monitor V2 — безопасно на поддерживаемых системах
        (void)g_pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        return;
    }

    // Дальше пробуем shcore (Windows 8.1) — Per-Monitor
    if (g_pSetProcessDpiAwareness)
    {
        const int PROCESS_PER_MONITOR_DPI_AWARE = 2;
        (void)g_pSetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        return;
    }

    // Наконец fallback: SetProcessDPIAware (Vista+), делает процесс system-DPI aware
    if (g_pSetProcessDPIAware)
    {
        (void)g_pSetProcessDPIAware();
        return;
    }

    // Если ничего нет (очень старые системы) — оставляем поведение по умолчанию (XP и т.п.)
}


// Получение текущего DPI для окна (безопасно для XP->Win11)
UINT GetEffectiveDpi(HWND hwnd)
{
    // Если InitDpi() не был вызван из WinMain, делаем минимальную инициализацию модулей.
    if (!g_dpiInitialized) InitDpi();

    // Windows 10+: GetDpiForWindow если доступна
    if (g_pGetDpiForWindow)
    {
        UINT dpi = g_pGetDpiForWindow(hwnd);
        if (dpi != 0) return dpi;
    }

    // Windows 8.1+: GetDpiForMonitor (через MonitorFromWindow)
    if (g_pGetDpiForMonitor)
    {
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        UINT dpiX = 0, dpiY = 0;
        if (SUCCEEDED(g_pGetDpiForMonitor(hMon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) && dpiX != 0)
            return dpiX;
    }

    // Fallback: классический GetDeviceCaps (XP, старые системы)
    HDC hdc = GetDC(NULL);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

    return (dpi > 0) ? (UINT)dpi : 96u;

}