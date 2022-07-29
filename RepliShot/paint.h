#pragma once

#include <Windows.h>
#include <WinUser.h>
#include <math.h>
#include <sstream>

#include "customtypes.h"
#include "clubreadingdata.h"

#define WINDOW_X_SIZE 340

#ifdef _DEBUG
#define WINDOW_Y_SIZE 530
#else
#define WINDOW_Y_SIZE 325
#endif

HBITMAP GetRotatedBitmapNT(HDC hdc, HBITMAP hBitmap, double radians, COLORREF clrBack);

void flipBitmap(HWND hWnd, HBITMAP bitmap);

void screenCaptureDebug(HDC hdc, int left, int top);

void drawGraphics(HWND hWnd);