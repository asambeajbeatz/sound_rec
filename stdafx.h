#pragma once

#define _CRT_SECURE_NO_WARNINGS

#define WINVER 0x0501       // for Windows XP
#define _WIN32_WINNT 0x0501 // for Windows XP

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <mmsystem.h>

#include <thread> // для мультипотока
#include <atomic> // для мультипотока
#include <mutex> // для мультипотока

#include <math.h>
#include <stdio.h>
#include <commctrl.h> // for trackbar
#include <cstdint> // для типов uint8_t, int32_t

//#pragma comment(lib, "comctl32.lib") 