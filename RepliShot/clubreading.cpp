#include "clubreading.h"

#include "customtypes.h"

extern struct Node* clubs;
extern struct Node* current_selected_club;
extern bool keep_polling;
extern bool club_lockstep;
extern int selected_club;
extern int selected_club_value;
extern HWND clubSelect;

extern std::mutex club_data;

void ReadFromScreen(RECT rc)
{
    //Capture part of screen
    HDC desktop_hdc = GetDC(NULL); // get the desktop device context
    HDC hDest = CreateCompatibleDC(desktop_hdc); // create a device context to use yourself

    //define portion of the screen
    RECT capture_area;
    HWND SomeWindowHandle = GetDesktopWindow();
    GetWindowRect(SomeWindowHandle, &capture_area);
    capture_area.left = capture_area.right * 4 / 5;
    capture_area.top = capture_area.bottom * 0.15;
    capture_area.bottom = capture_area.bottom * 0.19;

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
    //OutputDebugStringA(select_club.str().c_str());

    if (best_error > MINCLUBCHANGE) picked_club = -1;

    //std::ostringstream stros;
    //stros << "New Data!\n{ {";
    //for (y = 0; y < HBINS; y++) {
    //    for (x = 0; x < VBINS; x++) {
    //        stros << histogram[y][x];
    //        stros << ", ";
    //    }
    //    if (y < HBINS - 1) stros << "},\n{";
    //    else stros << "} }\n";
    //}
    //OutputDebugStringA(stros.str().c_str());

    club_data.lock();
    if (picked_club != -1 && picked_club != current_selected_club->club) {
        int attempts = 0;
        current_selected_club = clubs;
        while (current_selected_club->club != picked_club && attempts < 25) {
            current_selected_club = current_selected_club->next;
            attempts++;
        }
        selected_club = attempts;
        selected_club_value = current_selected_club->club;
        SendMessage(clubSelect, CB_SETCURSEL, (WPARAM)selected_club, (LPARAM)0);
    }
    club_data.unlock();

    ReleaseDC(NULL, desktop_hdc);
    DeleteDC(hDest);
    DeleteObject(hBitmap2);
}

void clubReading() {
    while (keep_polling) {
        if (club_lockstep) {
            RECT desktop;
            HWND SomeWindowHandle = GetDesktopWindow();
            GetWindowRect(SomeWindowHandle, &desktop);
            desktop.left = desktop.right * 3 / 4;
            desktop.bottom = desktop.bottom / 4;
            ReadFromScreen(desktop);
        }
        Sleep(1000);
    }
}

