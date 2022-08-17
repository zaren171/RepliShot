#include "paint.h"

extern HBITMAP golfball_bmap;
extern HBITMAP driver_bmap;
extern HBITMAP iron_bmap;
extern HBITMAP putter_bmap;
extern HBITMAP opti_bmap;

extern bool hostmode;
extern bool clientmode;
extern bool clientConnected;
extern bool opticonnected;
extern bool lefty;

extern std::string ip_addr;

extern double swing_angle;
extern int swing_path;
extern int face_contact;
extern int selected_club_value;

// GetRotatedBitmapNT - Create a new bitmap with rotated image
// Returns - Returns new bitmap with rotated image
// hBitmap - Bitmap to rotate
// radians - Angle of rotation in radians
// clrBack - Color of pixels in the resulting bitmap that do
// not get covered by source pixels
HBITMAP GetRotatedBitmapNT(HDC hdc, HBITMAP hBitmap, double radians, COLORREF clrBack)
{
    // Create a memory DC compatible with the display
    HDC sourceHDC = CreateCompatibleDC(hdc);
    HDC destHDC = CreateCompatibleDC(hdc);

    // Get logical coordinates
    BITMAP bm;
    GetObject(hBitmap, sizeof(bm), &bm);

    float cosine = (float)cos(radians);
    float sine = (float)sin(radians);

    // Compute dimensions of the resulting bitmap
    // First get the coordinates of the 3 corners other than origin
    int x1 = (int)(bm.bmHeight * sine);
    int y1 = (int)(bm.bmHeight * cosine);
    int x2 = (int)(bm.bmWidth * cosine + bm.bmHeight * sine);
    int y2 = (int)(bm.bmHeight * cosine - bm.bmWidth * sine);
    int x3 = (int)(bm.bmWidth * cosine);
    int y3 = (int)(-bm.bmWidth * sine);

    int minx = min(0, min(x1, min(x2, x3)));
    int miny = min(0, min(y1, min(y2, y3)));
    int maxx = max(0, max(x1, max(x2, x3)));
    int maxy = max(0, max(y1, max(y2, y3)));

    int w = maxx - minx;
    int h = maxy - miny;

    // Create a bitmap to hold the result
    HBITMAP hbmResult = CreateCompatibleBitmap(hdc, w, h);

    HBITMAP hbmOldSource = (HBITMAP)SelectObject(sourceHDC, hBitmap);
    HBITMAP hbmOldDest = (HBITMAP)SelectObject(destHDC, hbmResult);

    // Draw the background color before we change mapping mode
    HBRUSH hbrBack = CreateSolidBrush(clrBack);
    HBRUSH hbrOld = (HBRUSH)::SelectObject(destHDC, hbrBack);
    PatBlt(destHDC, 0, 0, w, h, PATCOPY);
    ::DeleteObject(::SelectObject(destHDC, hbrOld));

    // We will use world transform to rotate the bitmap
    SetGraphicsMode(destHDC, GM_ADVANCED);
    XFORM xform;
    xform.eM11 = cosine;
    xform.eM12 = -sine;
    xform.eM21 = sine;
    xform.eM22 = cosine;
    xform.eDx = (float)-minx;
    xform.eDy = (float)-miny;

    SetWorldTransform(destHDC, &xform);

    // Now do the actual rotating - a pixel at a time
    BitBlt(destHDC, 0, 0, bm.bmWidth, bm.bmHeight, sourceHDC, 0, 0, SRCCOPY);

    // Restore DCs
    SelectObject(sourceHDC, hbmOldSource);
    SelectObject(destHDC, hbmOldDest);

    return hbmResult;
}

void flipBitmap(HWND hWnd, HBITMAP bitmap) {
    //flip bitmaps for lefty mode toggle
    PAINTSTRUCT ps;
    BITMAP bm;
    HDC hdc;
    HDC sourceHDC;
    HBITMAP hbmOldSource;

    hdc = BeginPaint(hWnd, &ps);
    sourceHDC = CreateCompatibleDC(hdc);

    hbmOldSource = (HBITMAP)SelectObject(sourceHDC, bitmap);
    GetObject(bitmap, sizeof(bm), &bm);
    StretchBlt(sourceHDC, 0, 0, bm.bmWidth, bm.bmHeight, sourceHDC, 0, bm.bmHeight - 1, bm.bmWidth, -bm.bmHeight, SRCCOPY);
    SelectObject(sourceHDC, hbmOldSource);
}

void screenCaptureDebug(HDC hdc, int left, int top) {
    //Capture part of screen
    HDC desktop_hdc = GetDC(NULL); // get the desktop device context
    HDC hDest = CreateCompatibleDC(desktop_hdc); // create a device context to use yourself
    
    //define portion of the screen
    RECT capture_area;
    HWND SomeWindowHandle = GetDesktopWindow();
    GetWindowRect(SomeWindowHandle, &capture_area);
    capture_area.bottom = capture_area.bottom * 0.5;
    
    int x, y;
    int capture_width = capture_area.right - capture_area.left;
    int capture_height = capture_area.bottom - capture_area.top;
    
    BYTE* bitPointer;
    BITMAPINFO biInfo;
    biInfo.bmiHeader.biSize = sizeof(biInfo.bmiHeader);
    biInfo.bmiHeader.biWidth = capture_width;
    biInfo.bmiHeader.biHeight = capture_height;
    biInfo.bmiHeader.biPlanes = 1;
    biInfo.bmiHeader.biBitCount = 32;
    biInfo.bmiHeader.biCompression = BI_RGB;
    biInfo.bmiHeader.biSizeImage = capture_width * 4 * capture_height;
    biInfo.bmiHeader.biClrUsed = 0;
    biInfo.bmiHeader.biClrImportant = 0;
    HBITMAP hBitmap2 = CreateDIBSection(hDest, &biInfo, DIB_RGB_COLORS, (void**)(&bitPointer), NULL, NULL);
    SelectObject(hDest, hBitmap2);
    BitBlt(hDest, 0, 0, capture_width, capture_height, desktop_hdc, capture_area.left, capture_area.top, SRCCOPY);
    
    //box detection variables
    int min_len = 30;
    int found_vlines = 0;
    int found_hlines = 0;
    int line_len = 0;

    int bl_corners = 0;
    int tr_corners = 0;
    //find horizontal lines
    for (y = min_len; y < capture_height-min_len; y++) {
        for (x = 0; x < capture_width; x++) {
            int pixel = (x + (y * capture_width)) * 4;
            if ((bitPointer[pixel] > THRESHOLD) && (bitPointer[pixel + 1] > THRESHOLD) && (bitPointer[pixel + 2] > THRESHOLD)) {
                line_len++;
            }
            else {
                line_len = 0;
            }
            if (line_len >= min_len && line_len <= min_len + 2) {
                bool bl_corner = TRUE;
                for (int i = 2; i < min_len; i++) {
                    int test_pixel = ((x - (min_len+2)) + ((y + i) * capture_width)) * 4;
                    if (!((bitPointer[test_pixel] > THRESHOLD) && (bitPointer[test_pixel + 1] > THRESHOLD) && (bitPointer[test_pixel + 2] > THRESHOLD))) {
                        bl_corner = FALSE;
                    }
                    test_pixel = ((x - (min_len + 1)) + ((y + i) * capture_width)) * 4;
                    if ((bitPointer[test_pixel] > THRESHOLD) && (bitPointer[test_pixel + 1] > THRESHOLD) && (bitPointer[test_pixel + 2] > THRESHOLD)) {
                        bl_corner = FALSE;
                    }
                }
                if (bl_corner) {
                    bitPointer[pixel] = 0;
                    bitPointer[pixel + 1] = 0;
                    bitPointer[pixel + 2] = 255;
                    //for (int past_pixel = 1; past_pixel < min_len; past_pixel++) {
                    //    bitPointer[pixel - (past_pixel * 4)] = 0;
                    //    bitPointer[pixel - (past_pixel * 4) + 1] = 0;
                    //    bitPointer[pixel - (past_pixel * 4) + 2] = 255;
                    //}
                    //for (int i = 2; i < min_len; i++) {
                    //    int test_pixel = ((x - (min_len + 2)) + ((y + i) * capture_width)) * 4;
                    //    bitPointer[test_pixel] = 0;
                    //    bitPointer[test_pixel + 1] = 0;
                    //    bitPointer[test_pixel + 2] = 255;
                    //}
                    bl_corners++;
                }
            }
            if (line_len >= min_len) {
                bool tr_corner = TRUE;
                for (int i = 2; i < min_len; i++) {
                    int test_pixel = ((x + 2) + ((y - i) * capture_width)) * 4;
                    if (!((bitPointer[test_pixel] > THRESHOLD) && (bitPointer[test_pixel + 1] > THRESHOLD) && (bitPointer[test_pixel + 2] > THRESHOLD))) {
                        tr_corner = FALSE;
                    }
                    test_pixel = ((x + 1) + ((y - i) * capture_width)) * 4;
                    if ((bitPointer[test_pixel] > THRESHOLD) && (bitPointer[test_pixel + 1] > THRESHOLD) && (bitPointer[test_pixel + 2] > THRESHOLD)) {
                        tr_corner = FALSE;
                    }
                }
                if (tr_corner) {
                    bitPointer[pixel] = 0;
                    bitPointer[pixel + 1] = 255;
                    bitPointer[pixel + 2] = 0;
                    //for (int past_pixel = 1; past_pixel < min_len; past_pixel++) {
                    //    bitPointer[pixel - (past_pixel * 4)] = 0;
                    //    bitPointer[pixel - (past_pixel * 4) + 1] = 255;
                    //    bitPointer[pixel - (past_pixel * 4) + 2] = 0;
                    //}
                    //for (int i = 2; i < min_len; i++) {
                    //    int test_pixel = ((x + 2) + ((y - i) * capture_width)) * 4;
                    //    bitPointer[test_pixel] = 0;
                    //    bitPointer[test_pixel + 1] = 255;
                    //    bitPointer[test_pixel + 2] = 0;
                    //}
                    tr_corners++;
                }
            }
        }
    }
    std::ostringstream debug;
    debug << "bl: " << bl_corners << " tr: " << tr_corners << "\n";
    OutputDebugStringA(debug.str().c_str());

    //club detection stuff
    /*
    int bin_size = (capture_area.bottom - capture_area.top) / (HBINS - 1);
    int histogram[HBINS][VBINS];
    int intensities[3][5];
    for (x = 0; x < VBINS; x++) {
        for (y = 0; y < HBINS; y++) {
            histogram[y][x] = 0;
        }
    }
    for (x = 0; x < (capture_width * 4 * capture_height); x += 4) {
        if ((bitPointer[x] > THRESHOLD) && (bitPointer[x + 1] > THRESHOLD) && (bitPointer[x + 2] > THRESHOLD)) {
            histogram[((x / 4) / capture_width) / bin_size][((x / 4) % capture_width) / bin_size]++;
        }
    }
    int best_error = -1;
    int picked_club = -1;
    std::ostringstream select_club;
    for (int z = 0; z < CLUBS; z++) {
        int total_error = 0;
        for (x = 0; x < VBINS; x++) {
            for (y = 0; y < HBINS; y++) {
                total_error += abs(data::club_image_data_1080p[z][y][x] - histogram[y][x]);
            }
        }
        if (best_error < 0 || total_error < best_error) {
            best_error = total_error;
            picked_club = z;
            select_club.str("");
            select_club << "Club " << (int)z << " Selected! Error: " << total_error << "\n";
        }
    }
    OutputDebugStringA(select_club.str().c_str());
    
    if (best_error > MINCLUBCHANGE) picked_club = -1;
    
    std::ostringstream stros;
    stros << "New Data!\n{ {";
    for (y = 0; y < HBINS; y++) {
        for (x = 0; x < VBINS; x++) {
            stros << histogram[y][x];
            stros << ", ";
        }
        if (y < HBINS - 1) stros << "},\n{";
        else stros << "} }\n";
    }
    OutputDebugStringA(stros.str().c_str());
    */

    BitBlt(hdc, left, top, capture_area.right - capture_area.left, capture_area.bottom - capture_area.top, hDest, capture_area.left, capture_area.top, SRCCOPY);
    
    ReleaseDC(NULL, desktop_hdc);
    DeleteDC(hDest);
    DeleteObject(hBitmap2);
}

void drawGraphics(HWND hWnd){
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    BITMAP          bitmap;
    HDC             hdcMem;
    HGDIOBJ         oldBitmap;

    int top = WINDOW_Y_SIZE - 215;
    int left = 10;
    int height = 150;
    int width = 305;

    //screenCaptureDebug(hdc, left, top);

    //Draw Golf Ball
    hdcMem = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(hdcMem, golfball_bmap);

    GetObject(golfball_bmap, sizeof(bitmap), &bitmap);
    BitBlt(hdc, left + 135, top + 50, left + bitmap.bmWidth, top + bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);

    //Draw Club
    HBITMAP club_image = NULL;
    if (selected_club_value < I2) club_image = driver_bmap;
    else if (selected_club_value == Putter) club_image = putter_bmap;
    else club_image = iron_bmap;

    if (lefty) swing_angle += 180;
    club_image = GetRotatedBitmapNT(hdc, club_image, (swing_angle * PI / 180.0), COLORREF(0x00FFFFFF));

    hdcMem = CreateCompatibleDC(hdc);
    oldBitmap = SelectObject(hdcMem, club_image);

    GetObject(club_image, sizeof(bitmap), &bitmap);

    double path_offset = swing_path * .08333;
    if (lefty) BitBlt(hdc, left + 97 - bitmap.bmWidth, top + 25 + int(path_offset * -55) + (face_contact * 17), left + bitmap.bmWidth, top + bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
    else  BitBlt(hdc, left + 220, top + 25 + int(path_offset * 55) + (face_contact * 17), left + bitmap.bmWidth, top + bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, oldBitmap);
    DeleteDC(hdcMem);

    //Draw the path line
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    MoveToEx(hdc, 70, top + 75 - int(path_offset * 100), 0);
    LineTo(hdc, 270, top + 75 + int(path_offset * 100));
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    if (hostmode) {
        IPv4 ipaddr;
        if (getMyIP(ipaddr)) {
            std::stringstream s;
            s << "Hosting at: " << (int)ipaddr.b1 << "." << (int)ipaddr.b2 << "." << (int)ipaddr.b3 << "." << (int)ipaddr.b4;
            RECT rect;
            SetTextColor(hdc, 0x00000000);
            SetBkMode(hdc, TRANSPARENT);
            rect.left = left - 5;
            rect.top = top + 140;
            DrawTextA(hdc, s.str().c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);
        }
    }

    if (clientmode) {
        if (clientConnected) {
            HPEN ellipsePen = CreatePen(PS_SOLID, 10, RGB(0, 255, 0));
            SelectObject(hdc, ellipsePen);
            Ellipse(hdc, left, top + 145, left + 5, top + 150);
            std::stringstream s;
            s << "Connected to: " << ip_addr;
            RECT rect;
            SetTextColor(hdc, 0x00000000);
            SetBkMode(hdc, TRANSPARENT);
            rect.left = left + 15;
            rect.top = top + 140;
            DrawTextA(hdc, s.str().c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);
        }
        else {
            HPEN ellipsePen = CreatePen(PS_SOLID, 10, RGB(255, 0, 0));
            SelectObject(hdc, ellipsePen);
            Ellipse(hdc, left, top + 145, left + 5, top + 150);
            std::stringstream s;
            s << "No Connection";
            RECT rect;
            SetTextColor(hdc, 0x00000000);
            SetBkMode(hdc, TRANSPARENT);
            rect.left = left + 15;
            rect.top = top + 140;
            DrawTextA(hdc, s.str().c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);
        }
    }

    //Draw Optipad if connected
    if (opticonnected) {
        hdcMem = CreateCompatibleDC(hdc);
        oldBitmap = SelectObject(hdcMem, opti_bmap);

        GetObject(golfball_bmap, sizeof(bitmap), &bitmap);
        BitBlt(hdc, left + 295, top + 140, left + bitmap.bmWidth, top + bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, oldBitmap);
        DeleteDC(hdcMem);
    }

    EndPaint(hWnd, &ps);
}