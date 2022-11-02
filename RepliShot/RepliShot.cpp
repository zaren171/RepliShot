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
#include "configfile.h"
#include "paint.h"

#define MAX_LOADSTRING 100

//TODO:
//multiscreen: reading the right region for club info with multiple monitors
//resolution: reading the right region for club info at different apsect ratios
//resolution: scaling club reading data, or gathering new data for different resolutions
//network code: fix it so it isn't so fragile

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
HWND arcadeValue = NULL;
HWND arcadeUp = NULL;
HWND arcadeDown = NULL;
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

double arcade_mult = 1.5;

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
bool arcade_mode = FALSE;

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

    arcadeValue = CreateWindowW(TEXT("Static"), TEXT("1.5"), WS_CHILD, 150, y_offset, 30, 20, hWnd, NULL, NULL, NULL);
    arcadeUp = CreateWindowW(TEXT("BUTTON"), TEXT("+"), WS_CHILD | WS_BORDER | BS_DEFPUSHBUTTON, 180, y_offset, 30, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    arcadeDown = CreateWindowW(TEXT("BUTTON"), TEXT("-"), WS_CHILD | WS_BORDER | BS_DEFPUSHBUTTON, 120, y_offset, 30, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

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

    readconfig(hWnd);

    return TRUE;
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
        else if (lParam == LPARAM(arcadeUp)) {
            if (arcade_mult < 9.9) {
                arcade_mult = arcade_mult + 0.05;
                std::wstringstream wss;
                wss << arcade_mult;
                SetWindowText(arcadeValue, wss.str().c_str());
            }
        }
        else if (lParam == LPARAM(arcadeDown)) {
            if (arcade_mult > 0.1) {
                arcade_mult = arcade_mult - 0.05;
                std::wstringstream wss;
                wss << arcade_mult;
                SetWindowText(arcadeValue, wss.str().c_str());
            }
        }
        
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
            flipBitmap(hWnd, driver_bmap);
            flipBitmap(hWnd, iron_bmap);
            flipBitmap(hWnd, putter_bmap);
            break;
        case ID_OPTIONS_ARCADEMODE:
            arcade_mode = !arcade_mode;

            GetMenuItemInfo(hmenu, ID_OPTIONS_ARCADEMODE, FALSE, &menuItem);

            if (menuItem.fState == MFS_CHECKED) {
                // Checked, uncheck it
                menuItem.fState = MFS_UNCHECKED;
                ShowWindow(arcadeUp, SW_HIDE);
                ShowWindow(arcadeDown, SW_HIDE);
                ShowWindow(arcadeValue, SW_HIDE);
            }
            else {
                // Unchecked, check it
                menuItem.fState = MFS_CHECKED;
                ShowWindow(arcadeUp, SW_NORMAL);
                ShowWindow(arcadeDown, SW_NORMAL);
                ShowWindow(arcadeValue, SW_NORMAL);
            }
            SetMenuItemInfo(hmenu, ID_OPTIONS_ARCADEMODE, FALSE, &menuItem);

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
            saveconfig();
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
        drawGraphics(hWnd);
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
