// dpi_utils.h

#pragma once

#include <windows.h>

// Инициализация DPI-awareness (вызывать один раз в WinMain до CreateWindow)
void InitDpi(void);

// Получение текущего DPI окна (XP -> Windows 11)
UINT GetEffectiveDpi(HWND hwnd);