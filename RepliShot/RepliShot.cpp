// Mouse_Controller.cpp : Defines the entry point for the application.
//

//these seem to have to be at the top for things to build... not sure why
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
//end things that have to be at the top

#include "framework.h"
#include "RepliShot.h"
#include <Windows.h>
#include <WinUser.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <windowsx.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <chrono>
#include <thread>
#include <math.h>
#include <algorithm>
#include <vector>
#include <mutex>

#include "hidapi.h"
#include "libusb.h"

#include "networkcode.h"
#include "usbcode.h"
#include "clubreading.h"
#include "customtypes.h"

#define MAX_LOADSTRING 100

#define WINDOW_X_SIZE 340

#ifdef _DEBUG
#define WINDOW_Y_SIZE 530
#else
#define WINDOW_Y_SIZE 325
#endif


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//GUI elements
HWND mainWindow = NULL;
HWND clubSelect = NULL;
HWND reswingButton = NULL;
HWND clickmouseValue = NULL;
HWND usingballValue = NULL;
HWND clubSpeedValue = NULL;
HWND faceAngleValue = NULL;
HWND pathValue = NULL;
HWND faceContactValue = NULL;
#ifdef _DEBUG
HWND backswingstepsizeValue = NULL;
HWND backswingstepsValue = NULL;
HWND midswingdelayValue = NULL;
HWND forwardswingstepsizeValue = NULL;
HWND forwardswingstepsValue = NULL;
HWND slopeValue = NULL;
HWND sideaccelValue = NULL;
HWND sidescaleValue = NULL;
HWND collectshotValue = NULL;
HWND replayData = NULL;
HWND replayButton = NULL;
#endif

HBITMAP golfball_bmap = NULL;
HBITMAP driver_bmap = NULL;
HBITMAP iron_bmap = NULL;
HBITMAP putter_bmap = NULL;
HBITMAP opti_bmap = NULL;

uint8_t* report_buffer;
hid_device* dev = NULL;
libusb_device_handle* handle = NULL;
std::vector<std::thread> threads;

FILE* fp;

SOCKET ClientSocket = INVALID_SOCKET;
SOCKET HostSocket = INVALID_SOCKET;

std::string ip_addr = "127.0.0.1";

std::mutex club_data;

int desktop_width = 0;
int desktop_height = 0;

int selected_club = 0;
int selected_club_value = 0;

double backswingstepsize = 7;
double backswingsteps = 45;
int midswingdelay = 625;
double forwardswingstepsize = 25;
double forwardswingsteps = 30;
double slope = 0.0;
double sideaccel = 1.05;
double sidescale = 0.0;
bool clickMouse = FALSE;
bool usingball = TRUE;

double swing_angle = 0.0;
int swing_path = 0;
int face_contact = 0;

bool logging = FALSE; //write raw shot data to data_log.txt

bool hostmode = FALSE;
bool clientmode = FALSE;
bool clientConnected = FALSE;
bool opticonnected = FALSE;
bool collect_swing = FALSE;
bool keep_polling = TRUE;
bool on_top = FALSE;
bool opti_connected = FALSE;
bool lefty = FALSE;
bool club_lockstep = FALSE;
bool driving_range = FALSE;
bool front_sensor_features = FALSE;

struct Node* clubs = NULL;
struct Node* current_selected_club = NULL;
int num_clubs = -1;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    desktop_width = desktop.right;
    desktop_height = desktop.bottom; 

    threads.push_back(std::thread(clubReading));
    
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_REPLISHOT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REPLISHOT));

    MSG msg;

    /////////////////////////////////////////////// USB CODE ////////////

    int r, loop = 0;
    int size = 60;
    report_buffer = (uint8_t*)calloc(size, 1);

    int retry = 10;
    retry = opti_connect();

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    keep_polling = FALSE;

    shutdown(ClientSocket, SD_SEND);
    closesocket(ClientSocket);
    shutdown(HostSocket, SD_SEND);
    closesocket(HostSocket);
    WSACleanup();

    for (auto& th : threads) th.join();

    if (handle != NULL) {
        r = opti_shutdown(handle, 1, report_buffer, size);
        if (r != 0) return r;
        //////// END OPTI-CONTROL ///////////

        hid_close(dev);
        libusb_close(handle);
    }

    libusb_exit(NULL);
    if(fp) fclose(fp);

    if(handle != NULL) return (int)msg.wParam;
    return 0;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_REPLISHOT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_REPLISHOT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_X_SIZE, WINDOW_Y_SIZE, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    mainWindow = hWnd;

    int y_offset = 10;
#ifdef _DEBUG
    CreateWindowW(TEXT("Static"), TEXT("Backswing Step Size"), WS_CHILD | WS_VISIBLE, 10, y_offset, 200, 20, hWnd, NULL, NULL, NULL);
    backswingstepsizeValue = CreateWindowW(TEXT("Edit"), TEXT("6"), WS_CHILD | WS_VISIBLE | WS_BORDER, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 20;
    CreateWindowW(TEXT("Static"), TEXT("Backswing Steps"), WS_CHILD | WS_VISIBLE, 10, y_offset, 200, 20, hWnd, NULL, NULL, NULL);
    backswingstepsValue = CreateWindowW(TEXT("Edit"), TEXT("45"), WS_CHILD | WS_VISIBLE | WS_BORDER, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 20;

    CreateWindowW(TEXT("Static"), TEXT("Midswing Delay"), WS_CHILD | WS_VISIBLE, 10, y_offset, 200, 20, hWnd, NULL, NULL, NULL);
    midswingdelayValue = CreateWindowW(TEXT("Edit"), TEXT("625"), WS_CHILD | WS_VISIBLE | WS_BORDER, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 20;

    CreateWindowW(TEXT("Static"), TEXT("Frontswing Step Size"), WS_CHILD | WS_VISIBLE, 10, y_offset, 200, 20, hWnd, NULL, NULL, NULL);
    forwardswingstepsizeValue = CreateWindowW(TEXT("Edit"), TEXT("25"), WS_CHILD | WS_VISIBLE | WS_BORDER, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 20;
    CreateWindowW(TEXT("Static"), TEXT("Frongswing Steps"), WS_CHILD | WS_VISIBLE, 10, y_offset, 200, 20, hWnd, NULL, NULL, NULL);
    forwardswingstepsValue = CreateWindowW(TEXT("Edit"), TEXT("30"), WS_CHILD | WS_VISIBLE | WS_BORDER, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 20;

    CreateWindowW(TEXT("Static"), TEXT("Slope"), WS_CHILD | WS_VISIBLE, 10, y_offset, 200, 20, hWnd, NULL, NULL, NULL);
    slopeValue = CreateWindowW(TEXT("Edit"), TEXT("0.0"), WS_CHILD | WS_VISIBLE | WS_BORDER, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 20;

    CreateWindowW(TEXT("Static"), TEXT("Side Accel"), WS_CHILD | WS_VISIBLE, 10, y_offset, 200, 20, hWnd, NULL, NULL, NULL);
    sideaccelValue = CreateWindowW(TEXT("Edit"), TEXT("1.05"), WS_CHILD | WS_VISIBLE | WS_BORDER, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 20;
    CreateWindowW(TEXT("Static"), TEXT("Side Accel Scale"), WS_CHILD | WS_VISIBLE, 10, y_offset, 200, 20, hWnd, NULL, NULL, NULL);
    sidescaleValue = CreateWindowW(TEXT("Edit"), TEXT("0.0"), WS_CHILD | WS_VISIBLE | WS_BORDER, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 30;
    collectshotValue = CreateWindowW(TEXT("BUTTON"), TEXT("Collect Shots"), WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTOCHECKBOX, 10, y_offset, 100, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    replayData = CreateWindowW(TEXT("Edit"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 110, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    replayButton = CreateWindowW(TEXT("BUTTON"), TEXT("Replay Shot"), WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 210, y_offset, 100, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    y_offset += 30;

#endif

    clickmouseValue = CreateWindowW(TEXT("BUTTON"), TEXT("Take Shot"), WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTOCHECKBOX, 10, y_offset, 100, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    usingballValue = CreateWindowW(TEXT("BUTTON"), TEXT("Using Ball"), WS_CHILD | WS_VISIBLE | WS_BORDER | BS_AUTOCHECKBOX, 10, y_offset+20, 100, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    Button_SetCheck(usingballValue, BST_CHECKED);

    CreateWindowW(TEXT("Static"), TEXT("Club Speed:"), WS_CHILD | WS_VISIBLE, 115, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    clubSpeedValue = CreateWindowW(TEXT("Static"), TEXT("0.0"), WS_CHILD | WS_VISIBLE, 215, y_offset, 100, 20, hWnd, NULL, NULL, NULL);
    CreateWindowW(TEXT("Static"), TEXT("Face Angle:"), WS_CHILD | WS_VISIBLE, 115, y_offset+25, 100, 20, hWnd, NULL, NULL, NULL);
    faceAngleValue = CreateWindowW(TEXT("Static"), TEXT("0.0"), WS_CHILD | WS_VISIBLE, 215, y_offset+25, 100, 20, hWnd, NULL, NULL, NULL);
    CreateWindowW(TEXT("Static"), TEXT("Path:"), WS_CHILD | WS_VISIBLE, 115, y_offset+50, 100, 20, hWnd, NULL, NULL, NULL);
    pathValue = CreateWindowW(TEXT("Static"), TEXT("0.0"), WS_CHILD | WS_VISIBLE, 215, y_offset+50, 100, 20, hWnd, NULL, NULL, NULL);
    CreateWindowW(TEXT("Static"), TEXT("Face Contact:"), WS_CHILD | WS_VISIBLE, 115, y_offset+75, 100, 20, hWnd, NULL, NULL, NULL);
    faceContactValue = CreateWindowW(TEXT("Static"), TEXT("0.0"), WS_CHILD | WS_VISIBLE, 215, y_offset+75, 100, 20, hWnd, NULL, NULL, NULL);
    y_offset += 40;

    reswingButton = CreateWindowW(TEXT("BUTTON"), TEXT("Reswing"), WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 10, y_offset, 100, 30, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    y_offset += 30;

    clubSelect = CreateWindowW(TEXT("COMBOBOX"), TEXT("Clubs"), WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, 10, y_offset, 100, 200, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    y_offset += 30;

    TCHAR clublist[14][16] =
    {
        TEXT("Driver"), TEXT("3 Wood"), TEXT("5 Wood"), TEXT("5 Hybrid"),
        TEXT("5 Iron"), TEXT("6 Iron"), TEXT("7 Iron"), TEXT("8 Iron"), TEXT("9 Iron"),
        TEXT("Pitching Wedge"), TEXT("Gap Wedge"), TEXT("Sand Wedge"), TEXT("Lob Wedge"), TEXT("Putter")
    };

    TCHAR A[16];
    int  k = 0;

    memset(&A, 0, sizeof(A));
    for (k = 0; k < 14; k += 1)
    {
        wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)clublist[k]);

        // Add string to combobox.
        SendMessage(clubSelect, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
    }

    // Send the CB_SETCURSEL message to display an initial item 
    //  in the selection field  
    SendMessage(clubSelect, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);


    // std::ifstream is RAII, i.e. no need to call close
    std::ifstream cFile("config.ini");
    if (cFile.is_open())
    {
        std::string line;
        while (getline(cFile, line))
        {
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                line.end());
            if (line.empty() || line[0] == '#')
            {
                continue;
            }
            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);

            if (!(name.compare("Ball") || value.compare("False"))) {
                usingball = FALSE;
                Button_SetCheck(usingballValue, BST_UNCHECKED);
            }
            else if (!(name.compare("Shot") || value.compare("True"))) {
                clickMouse = TRUE;
                Button_SetCheck(clickmouseValue, BST_CHECKED);
            }
            else if (!(name.compare("Top") || value.compare("True"))) {
                on_top = TRUE;
                SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_FILE_KEEPONTOP, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_FILE_KEEPONTOP, FALSE, &menuItem);
            }
            else if (!(name.compare("Lefty") || value.compare("True"))) {
                lefty = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_FILE_LEFTHANDMODE, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_FILE_LEFTHANDMODE, FALSE, &menuItem);
            }
            else if (!(name.compare("Lockstep") || value.compare("True"))) {
                club_lockstep = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_OPTIONS_LOCKSTEPMODE, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_OPTIONS_LOCKSTEPMODE, FALSE, &menuItem);
            }
            else if (!(name.compare("DrivingRange") || value.compare("True"))) {
                driving_range = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_OPTIONS_DRIVINGRANGEMODE, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_OPTIONS_DRIVINGRANGEMODE, FALSE, &menuItem);
            }
            else if (!(name.compare("FrontSensor") || value.compare("True"))) {
                front_sensor_features = TRUE;
                HMENU hmenu = GetMenu(hWnd);
                MENUITEMINFO menuItem = { 0 };
                menuItem.cbSize = sizeof(MENUITEMINFO);
                menuItem.fMask = MIIM_STATE;
                GetMenuItemInfo(hmenu, ID_OPTIONS_FRONTSENSORFEATURES, FALSE, &menuItem);
                menuItem.fState = MFS_CHECKED;
                SetMenuItemInfo(hmenu, ID_OPTIONS_FRONTSENSORFEATURES, FALSE, &menuItem);
            }
            else if (!(name.compare("IP"))) {
                ip_addr = value;
            }
            else if (!(name.compare("Driver") || value.compare("True"))) {
                insertNode(&clubs, Driver);
            }
            else if (!(name.compare("2Wood") || value.compare("True"))) {
                insertNode(&clubs, W2);
            }
            else if (!(name.compare("3Wood") || value.compare("True"))) {
                insertNode(&clubs, W3);
            }
            else if (!(name.compare("4Wood") || value.compare("True"))) {
                insertNode(&clubs, W4);
            }
            else if (!(name.compare("5Wood") || value.compare("True"))) {
                insertNode(&clubs, W5);
            }
            else if (!(name.compare("2Hybrid") || value.compare("True"))) {
                insertNode(&clubs, HY2);
            }
            else if (!(name.compare("3Hybrid") || value.compare("True"))) {
                insertNode(&clubs, HY3);
            }
            else if (!(name.compare("4Hybrid") || value.compare("True"))) {
                insertNode(&clubs, HY4);
            }
            else if (!(name.compare("5Hybrid") || value.compare("True"))) {
                insertNode(&clubs, HY5);
            }
            else if (!(name.compare("2Iron") || value.compare("True"))) {
                insertNode(&clubs, I2);
            }
            else if (!(name.compare("3Iron") || value.compare("True"))) {
                insertNode(&clubs, I3);
            }
            else if (!(name.compare("4Iron") || value.compare("True"))) {
                insertNode(&clubs, I4);
            }
            else if (!(name.compare("5Iron") || value.compare("True"))) {
                insertNode(&clubs, I5);
            }
            else if (!(name.compare("6Iron") || value.compare("True"))) {
                insertNode(&clubs, I6);
            }
            else if (!(name.compare("7Iron") || value.compare("True"))) {
                insertNode(&clubs, I7);
            }
            else if (!(name.compare("8Iron") || value.compare("True"))) {
                insertNode(&clubs, I8);
            }
            else if (!(name.compare("9Iron") || value.compare("True"))) {
                insertNode(&clubs, I9);
            }
            else if (!(name.compare("Pitch") || value.compare("True"))) {
                insertNode(&clubs, PW);
            }
            else if (!(name.compare("Gap") || value.compare("True"))) {
                insertNode(&clubs, GW);
            }
            else if (!(name.compare("Sand") || value.compare("True"))) {
                insertNode(&clubs, SW);
            }
            else if (!(name.compare("Lob") || value.compare("True"))) {
                insertNode(&clubs, LW);
            }
            else if (!(name.compare("Putter") || value.compare("True"))) {
                insertNode(&clubs, Putter);
            }
        }

        current_selected_club = clubs;

        SendMessage(clubSelect, CB_RESETCONTENT, 0, 0);

        TCHAR A[16];
        int  k = 0;

        Node* currentclub = clubs;
        club clubToAdd = getClubData(currentclub->club);
        selected_club_value = currentclub->club;

        memset(&A, 0, sizeof(A));
        wcscpy_s(A, sizeof(A) / sizeof(TCHAR), clubToAdd.name);

        // Add string to combobox.
        SendMessage(clubSelect, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
        num_clubs++;

        currentclub = currentclub->next;

        while(currentclub->club != clubs->club)
        {
            clubToAdd = getClubData(currentclub->club);

            wcscpy_s(A, sizeof(A) / sizeof(TCHAR), clubToAdd.name);

            // Add string to combobox.
            SendMessage(clubSelect, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
            num_clubs++;

            currentclub = currentclub->next;
        }

        // Send the CB_SETCURSEL message to display an initial item 
        //  in the selection field  
        SendMessage(clubSelect, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    }
    else {
        insertNode(&clubs, Driver);
        insertNode(&clubs, W3);
        insertNode(&clubs, W5);
        insertNode(&clubs, HY5);
        insertNode(&clubs, I5);
        insertNode(&clubs, I6);
        insertNode(&clubs, I7);
        insertNode(&clubs, I8);
        insertNode(&clubs, I9);
        insertNode(&clubs, PW);
        insertNode(&clubs, GW);
        insertNode(&clubs, SW);
        insertNode(&clubs, LW);
        insertNode(&clubs, Putter);
        num_clubs = 13;
        current_selected_club = clubs;
    }

    return TRUE;
}

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

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        HMENU hmenu = GetMenu(hWnd);
        MENUITEMINFO menuItem = { 0 };
        menuItem.cbSize = sizeof(MENUITEMINFO);
        menuItem.fMask = MIIM_STATE;

#ifdef _DEBUG
        wchar_t text_buffer[20] = { 0 }; //temporary buffer
        swprintf(text_buffer, _countof(text_buffer), L"%d ", WORD(lParam)); // convert
        OutputDebugString(text_buffer); // print
        swprintf(text_buffer, _countof(text_buffer), L"%d ", GetDlgCtrlID(reswingButton)); // convert
        OutputDebugString(text_buffer); // print
        swprintf(text_buffer, _countof(text_buffer), L"%d\n", GetDlgCtrlID(hWnd)); // convert
        OutputDebugString(text_buffer); // print
#endif

        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            club_data.lock();
            selected_club = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
            current_selected_club = clubs;
            for (int i = 0; i < selected_club; i++) {
                current_selected_club = current_selected_club->next;
            }
            selected_club_value = current_selected_club->club;
            TCHAR  ListItem[256];
            (TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)selected_club, (LPARAM)ListItem);
            club_data.unlock();
        }

#ifdef _DEBUG
        if (lParam == LPARAM(backswingstepsizeValue)) {
            TCHAR buff[1024];
            LPTSTR endPtr;
            GetWindowText(backswingstepsizeValue, buff, 1024);
            backswingstepsize = _tcstod(buff, &endPtr);
        }
        else if (lParam == LPARAM(backswingstepsValue)) {
            TCHAR buff[1024];
            GetWindowText(backswingstepsValue, buff, 1024);
            backswingsteps = _wtoi(buff);
        }
        else if (lParam == LPARAM(midswingdelayValue)) {
            TCHAR buff[1024];
            GetWindowText(midswingdelayValue, buff, 1024);
            midswingdelay = _wtoi(buff);
        }
        else if (lParam == LPARAM(forwardswingstepsizeValue)) {
            TCHAR buff[1024];
            LPTSTR endPtr;
            GetWindowText(forwardswingstepsizeValue, buff, 1024);
            forwardswingstepsize = _tcstod(buff, &endPtr);
        }
        else if (lParam == LPARAM(forwardswingstepsValue)) {
            TCHAR buff[1024];
            GetWindowText(forwardswingstepsValue, buff, 1024);
            forwardswingsteps = _wtoi(buff);
        }
        else if (lParam == LPARAM(slopeValue)) {
            TCHAR buff[1024];
            LPTSTR endPtr;
            GetWindowText(slopeValue, buff, 1024);
            slope = _tcstod(buff, &endPtr);
        }
        else if (lParam == LPARAM(sideaccelValue)) {
            TCHAR buff[1024];
            LPTSTR endPtr;
            GetWindowText(sideaccelValue, buff, 1024);
            sideaccel = _tcstod(buff, &endPtr);
        }
        else if (lParam == LPARAM(sidescaleValue)) {
            TCHAR buff[1024];
            LPTSTR endPtr;
            GetWindowText(sidescaleValue, buff, 1024);
            sidescale = _tcstod(buff, &endPtr);
        }
        else if (lParam == LPARAM(replayButton)) {
            TCHAR buff[1024];
            LPTSTR endPtr;
            GetWindowText(replayData, buff, 1024);
            int data_size = 60;
            uint8_t* data;
            data = (uint8_t*)calloc(data_size, 1);
            TCHAR* pch;
            wchar_t* context;
            pch = wcstok_s(buff, L",", &context);
            int i = 0;
            bool fullDataSet = FALSE;
            while (pch != NULL && i < data_size) {
                data[i] = _wtoi(pch);
                pch = wcstok_s(NULL, L",", &context);
                i++;
                if (i == 60) fullDataSet = TRUE;
            }
            if (fullDataSet) {
                processShotData(data, data_size);
                takeShot();
            }
        }
        else if (lParam == LPARAM(collectshotValue)) {
            TCHAR buff[1024];
            GetWindowText(collectshotValue, buff, 1024);
            logging = !logging;
        }
        else if (lParam == LPARAM(clickmouseValue)) {
#else
        if (lParam == LPARAM(clickmouseValue)) {
#endif
            TCHAR buff[1024];
            GetWindowText(clickmouseValue, buff, 1024);
            clickMouse = !clickMouse;
        }
        else if (lParam == LPARAM(usingballValue)) {
            TCHAR buff[1024];
            GetWindowText(usingballValue, buff, 1024);
            usingball = !usingball;
        }
        else if (lParam == LPARAM(reswingButton)) {
            takeShot();
        }

        //save config variable
        std::ofstream cfgfile;
        
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case ID_FILE_KEEPONTOP:
            if (on_top) {
                SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                on_top = FALSE;
            }
            else {
                SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                on_top = TRUE;
            }

            GetMenuItemInfo(hmenu, ID_FILE_KEEPONTOP, FALSE, &menuItem);

            if (menuItem.fState == MFS_CHECKED) {
                // Checked, uncheck it
                menuItem.fState = MFS_UNCHECKED;
            }
            else {
                // Unchecked, check it
                menuItem.fState = MFS_CHECKED;
            }
            SetMenuItemInfo(hmenu, ID_FILE_KEEPONTOP, FALSE, &menuItem);

            break;
        case ID_FILE_LEFTHANDMODE:
            lefty = !lefty;

            GetMenuItemInfo(hmenu, ID_FILE_LEFTHANDMODE, FALSE, &menuItem);

            if (menuItem.fState == MFS_CHECKED) {
                // Checked, uncheck it
                menuItem.fState = MFS_UNCHECKED;
            }
            else {
                // Unchecked, check it
                menuItem.fState = MFS_CHECKED;
            }
            SetMenuItemInfo(hmenu, ID_FILE_LEFTHANDMODE, FALSE, &menuItem);

            //flip bitmaps for lefty mode toggle
            PAINTSTRUCT ps;
            BITMAP bm;
            HDC hdc;
            HDC sourceHDC;
            HBITMAP hbmOldSource;

            hdc = BeginPaint(hWnd, &ps);
            sourceHDC = CreateCompatibleDC(hdc);

            hbmOldSource = (HBITMAP)SelectObject(sourceHDC, driver_bmap);
            GetObject(driver_bmap, sizeof(bm), &bm);
            StretchBlt(sourceHDC, 0, 0, bm.bmWidth, bm.bmHeight, sourceHDC, 0, bm.bmHeight - 1, bm.bmWidth, -bm.bmHeight, SRCCOPY);
            SelectObject(sourceHDC, hbmOldSource);

            hbmOldSource = (HBITMAP)SelectObject(sourceHDC, iron_bmap);
            GetObject(driver_bmap, sizeof(bm), &bm);
            StretchBlt(sourceHDC, 0, 0, bm.bmWidth, bm.bmHeight, sourceHDC, 0, bm.bmHeight - 1, bm.bmWidth, -bm.bmHeight, SRCCOPY);
            SelectObject(sourceHDC, hbmOldSource);

            hbmOldSource = (HBITMAP)SelectObject(sourceHDC, putter_bmap);
            GetObject(driver_bmap, sizeof(bm), &bm);
            StretchBlt(sourceHDC, 0, 0, bm.bmWidth, bm.bmHeight, sourceHDC, 0, bm.bmHeight - 1, bm.bmWidth, -bm.bmHeight, SRCCOPY);
            SelectObject(sourceHDC, hbmOldSource);

            break;
        case ID_OPTIONS_LOCKSTEPMODE:
            club_lockstep = !club_lockstep;

            GetMenuItemInfo(hmenu, ID_OPTIONS_LOCKSTEPMODE, FALSE, &menuItem);

            if (menuItem.fState == MFS_CHECKED) {
                // Checked, uncheck it
                menuItem.fState = MFS_UNCHECKED;
            }
            else {
                // Unchecked, check it
                menuItem.fState = MFS_CHECKED;
            }
            SetMenuItemInfo(hmenu, ID_OPTIONS_LOCKSTEPMODE, FALSE, &menuItem);

            break;
        case ID_OPTIONS_DRIVINGRANGEMODE:
            driving_range = !driving_range;

            GetMenuItemInfo(hmenu, ID_OPTIONS_DRIVINGRANGEMODE, FALSE, &menuItem);

            if (menuItem.fState == MFS_CHECKED) {
                // Checked, uncheck it
                menuItem.fState = MFS_UNCHECKED;
            }
            else {
                // Unchecked, check it
                menuItem.fState = MFS_CHECKED;
            }
            SetMenuItemInfo(hmenu, ID_OPTIONS_DRIVINGRANGEMODE, FALSE, &menuItem);

            break;
        case ID_OPTIONS_FRONTSENSORFEATURES:
            front_sensor_features = !front_sensor_features;

            GetMenuItemInfo(hmenu, ID_OPTIONS_FRONTSENSORFEATURES, FALSE, &menuItem);

            if (menuItem.fState == MFS_CHECKED) {
                // Checked, uncheck it
                menuItem.fState = MFS_UNCHECKED;
            }
            else {
                // Unchecked, check it
                menuItem.fState = MFS_CHECKED;
            }
            SetMenuItemInfo(hmenu, ID_OPTIONS_FRONTSENSORFEATURES, FALSE, &menuItem);

            break;
        case ID_FILE_RECONNECTOPTISHOT:
            opti_connect();
            break;
        case ID_NETWORKMODE_CLIENTMODE:
            clientmode = !clientmode;

            if (hostmode && clientmode) {
                hostmode = FALSE;
                GetMenuItemInfo(hmenu, ID_NETWORKMODE_HOSTMODE, FALSE, &menuItem);
                menuItem.fState = MFS_UNCHECKED;
                SetMenuItemInfo(hmenu, ID_NETWORKMODE_HOSTMODE, FALSE, &menuItem);
            }
            else if (clientmode) {
                threads.push_back(std::thread(network_stack));
            }
            if (clientmode) {

            }

            GetMenuItemInfo(hmenu, ID_NETWORKMODE_CLIENTMODE, FALSE, &menuItem);

            if (menuItem.fState == MFS_CHECKED) {
                // Checked, uncheck it
                menuItem.fState = MFS_UNCHECKED;
            }
            else {
                // Unchecked, check it
                menuItem.fState = MFS_CHECKED;
            }
            SetMenuItemInfo(hmenu, ID_NETWORKMODE_CLIENTMODE, FALSE, &menuItem);

            RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

            break;
        case ID_NETWORKMODE_HOSTMODE:
            hostmode = !hostmode;

            if (hostmode && clientmode) {
                clientmode = FALSE;
                GetMenuItemInfo(hmenu, ID_NETWORKMODE_CLIENTMODE, FALSE, &menuItem);
                menuItem.fState = MFS_UNCHECKED;
                SetMenuItemInfo(hmenu, ID_NETWORKMODE_CLIENTMODE, FALSE, &menuItem);
            }
            else if (hostmode) {
                threads.push_back(std::thread(network_stack));
            }

            GetMenuItemInfo(hmenu, ID_NETWORKMODE_HOSTMODE, FALSE, &menuItem);

            if (menuItem.fState == MFS_CHECKED) {
                // Checked, uncheck it
                menuItem.fState = MFS_UNCHECKED;
            }
            else {
                // Unchecked, check it
                menuItem.fState = MFS_CHECKED;
            }
            SetMenuItemInfo(hmenu, ID_NETWORKMODE_HOSTMODE, FALSE, &menuItem);

            RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

            break;
        case ID_FILE_SAVECONFIG:
            cfgfile.open("config.ini");
            //take shot
            if (clickMouse) cfgfile << "Shot=True\n";
            else cfgfile << "Shot=False\n";
            //use ball
            if (usingball) cfgfile << "Ball=True\n";
            else cfgfile << "Ball=False\n";
            //on top
            if (on_top) cfgfile << "Top=True\n";
            else cfgfile << "Top=False\n";
            //lefty
            if (lefty) cfgfile << "Lefty=True\n";
            else cfgfile << "Lefty=False\n";
            //lockstep
            if (club_lockstep) cfgfile << "Lockstep=True\n";
            else cfgfile << "Lockstep=False\n";
            //drivingrange
            if (driving_range) cfgfile << "DrivingRange=True\n";
            else cfgfile << "DrivingRange=False\n";
            //frontsensor
            if (front_sensor_features) cfgfile << "FrontSensor=True\n";
            else cfgfile << "FrontSensor=False\n";
            //host ip addr
            cfgfile << "IP=" << ip_addr << "\n";
            //clubs
            cfgfile << "Driver=True\n";
            cfgfile << "2Wood=False\n";
            cfgfile << "3Wood=True\n";
            cfgfile << "4Wood=False\n";
            cfgfile << "5Wood=True\n";
            cfgfile << "2Hybrid=False\n";
            cfgfile << "3Hybrid=False\n";
            cfgfile << "4Hybrid=False\n";
            cfgfile << "5Hybrid=True\n";
            cfgfile << "2Iron=False\n";
            cfgfile << "3Iron=False\n";
            cfgfile << "4Iron=False\n";
            cfgfile << "5Iron=True\n";
            cfgfile << "6Iron=True\n";
            cfgfile << "7Iron=True\n";
            cfgfile << "8Iron=True\n";
            cfgfile << "9Iron=True\n";
            cfgfile << "Pitch=True\n";
            cfgfile << "Gap=True\n";
            cfgfile << "Sand=True\n";
            cfgfile << "Lob=True\n";
            cfgfile << "Putter=True\n";
            cfgfile.close();
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_CREATE:
        golfball_bmap = LoadBitmap((HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(IDB_GOLF_BALL_BITMAP));
        driver_bmap = LoadBitmap((HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(IDB_DRIVER_BITMAP));
        iron_bmap = LoadBitmap((HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(IDB_IRON_BITMAP));
        putter_bmap = LoadBitmap((HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(IDB_PUTTER_BITMAP));
        opti_bmap = LoadBitmap((HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), MAKEINTRESOURCE(IDB_OPTI));
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        BITMAP          bitmap;
        HDC             hdcMem;
        HGDIOBJ         oldBitmap;

        int top = WINDOW_Y_SIZE - 215;
        int left = 10;
        int height = 150;
        int width = 305;

        ////Capture part of screen
        //HDC desktop_hdc = GetDC(NULL); // get the desktop device context
        //HDC hDest = CreateCompatibleDC(desktop_hdc); // create a device context to use yourself
        //
        ////define portion of the screen
        //RECT capture_area;
        //HWND SomeWindowHandle = GetDesktopWindow();
        //GetWindowRect(SomeWindowHandle, &capture_area);
        //capture_area.left = capture_area.right * 4 / 5;
        //capture_area.top = capture_area.bottom * 0.15;
        //capture_area.bottom = capture_area.bottom * 0.19;
        //
        //int x, y;
        //int capture_width = capture_area.right - capture_area.left;
        //int capture_height = capture_area.bottom - capture_area.top;
        //
        //BYTE* bitPointer;
        //BITMAPINFO biInfo;
        //biInfo.bmiHeader.biSize = sizeof(biInfo.bmiHeader);
        //biInfo.bmiHeader.biWidth = capture_width;
        //biInfo.bmiHeader.biHeight = capture_height;
        //biInfo.bmiHeader.biPlanes = 1;
        //biInfo.bmiHeader.biBitCount = 32;
        //biInfo.bmiHeader.biCompression = BI_RGB;
        //biInfo.bmiHeader.biSizeImage = capture_width * 4 * capture_height;
        //biInfo.bmiHeader.biClrUsed = 0;
        //biInfo.bmiHeader.biClrImportant = 0;
        //HBITMAP hBitmap2 = CreateDIBSection(hDest, &biInfo, DIB_RGB_COLORS, (void**)(&bitPointer), NULL, NULL);
        //SelectObject(hDest, hBitmap2);
        //BitBlt(hDest, 0, 0, capture_width, capture_height, desktop_hdc, capture_area.left, capture_area.top, SRCCOPY);
        //
        //int bin_size = (capture_area.bottom - capture_area.top) / (HBINS - 1);
        //int histogram[HBINS][VBINS];
        //int intensities[3][5];
        //for (x = 0; x < VBINS; x++) {
        //    for (y = 0; y < HBINS; y++) {
        //        histogram[y][x] = 0;
        //    }
        //}
        //for (x = 0; x < (capture_width * 4 * capture_height); x += 4) {
        //    if ((bitPointer[x] > THRESHOLD) && (bitPointer[x + 1] > THRESHOLD) && (bitPointer[x + 2] > THRESHOLD)) {
        //        histogram[((x / 4) / capture_width) / bin_size][((x / 4) % capture_width) / bin_size]++;
        //    }
        //}
        //int best_error = -1;
        //int picked_club = -1;
        //std::ostringstream select_club;
        //for (int z = 0; z < CLUBS; z++) {
        //    int total_error = 0;
        //    for (x = 0; x < VBINS; x++) {
        //        for (y = 0; y < HBINS; y++) {
        //            total_error += abs(data::club_image_data_1080p[z][y][x] - histogram[y][x]);
        //        }
        //    }
        //    if (best_error < 0 || total_error < best_error) {
        //        best_error = total_error;
        //        picked_club = z;
        //        select_club.str("");
        //        select_club << "Club " << (int)z << " Selected! Error: " << total_error << "\n";
        //    }
        //}
        //OutputDebugStringA(select_club.str().c_str());
        //
        //if (best_error > MINCLUBCHANGE) picked_club = -1;
        //
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
        //
        //BitBlt(hdc, left, top, capture_area.right - capture_area.left, capture_area.bottom - capture_area.top, desktop_hdc, capture_area.left, capture_area.top, SRCCOPY);
        //
        //ReleaseDC(NULL, desktop_hdc);
        //DeleteDC(hDest);

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
    break;
    case WM_DESTROY:
        DeleteObject(golfball_bmap);
        DeleteObject(driver_bmap);
        DeleteObject(iron_bmap);
        DeleteObject(putter_bmap);
        DeleteObject(opti_bmap);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
