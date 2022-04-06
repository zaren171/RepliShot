// Mouse_Controller.cpp : Defines the entry point for the application.
//

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

#include "hidapi.h"
#include "libusb.h"

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

int desktop_width = 0;
int desktop_height = 0;

int selected_club = 0;

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

FILE* fp;
bool logging = FALSE; //write raw shot data to data_log.txt

////////////////// USB CODE /////////////////////////////

#define SHOTSLEEPTIME 1000
#define SENSORSPACING 185
#define LEDSPACING 15
#define PI 3.14159265

static bool collect_swing = FALSE;
static bool keep_polling = TRUE;
static bool on_top = FALSE;
static bool opti_connected = FALSE;
static bool lefty = FALSE;

enum golf_club {Driver, W3, W5, HY, I3, I4, I5, I6, I7, I8, I9, PW, GW, SW, Putter};

struct club {
    double club_mass;
    double club_loft;
    double launch_angle;
    double club_speed;
    double smash_factor;
};

// Future versions of libusb will use usb_interface instead of interface
// in libusb_config_descriptor => catter for that
#define usb_interface interface

static void perr(char const* format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

static uint16_t VID, PID;

static libusb_device_handle* init_device(uint16_t vid, uint16_t pid, libusb_context* context)
{
    libusb_device_handle* handle;

    printf("Opening device %04X:%04X...\n", vid, pid);
    handle = libusb_open_device_with_vid_pid(context, vid, pid);

    return handle;
}

static int usb_init(libusb_device_handle* handle) {
    int r = 0;

    libusb_set_auto_detach_kernel_driver(handle, 1);

    r = libusb_claim_interface(handle, 0);
    if (r != LIBUSB_SUCCESS) {
        perr("   Failed.\n");
    }

    return r;
}

static int opti_red(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size) {
    int r = 0;

    collect_swing = FALSE;

    //Turn LED Red
    report_buffer[0] = 0x51;
    r = libusb_interrupt_transfer(handle, endpoint, report_buffer, size, &size, 5000);
    if (r != LIBUSB_SUCCESS) {
        perr("   Failed.\n");
    }

    return 0;
}

static int opti_green(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size) {
    int r = 0;

    //Turn LED Green
    report_buffer[0] = 0x52;
    r = libusb_interrupt_transfer(handle, endpoint, report_buffer, size, &size, 5000);
    if (r != LIBUSB_SUCCESS) {
        perr("   Failed.\n");
    }

    collect_swing = TRUE;

    return 0;
}

static int opti_cycle(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size, int sleep_time) {
    int r = 0;

    r = opti_red(handle, endpoint, report_buffer, size);

    Sleep(sleep_time);

    r = opti_green(handle, endpoint, report_buffer, size);

    return 0;
}

static int opti_init(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size) {
    int r = 0;

    r = usb_init(handle);
    if (r != 0) {
        return -1;
    }

    //Turns on the sensors
    report_buffer[0] = 0x50;
    r = libusb_interrupt_transfer(handle, endpoint, report_buffer, size, &size, 5000);
    if (r != 0) {
        return -1;
    }

    //Turns LED Green
    report_buffer[0] = 0x52;
    r = libusb_interrupt_transfer(handle, endpoint, report_buffer, size, &size, 5000);
    if (r != 0) {
        return -1;
    }

    r = opti_cycle(handle, endpoint, report_buffer, size, 0);
    if (r != 0) {
        return -1;
    }

    return r;
}

static int opti_shutdown(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size) {
    int r = 0;

    //Turns off sensors/LED
    report_buffer[0] = 0x80;
    r = libusb_interrupt_transfer(handle, endpoint, report_buffer, size, &size, 5000);
    if (r != LIBUSB_SUCCESS) {
        perr("   Failed.\n");
    }

    return 0;
}

static int flush_buffer(hid_device* dev, unsigned char* data, unsigned char* prev_data, size_t length) {
    bool new_packet = TRUE;

    while (new_packet) {
        int r = 0;
        bool same = TRUE;
        r = hid_read(dev, data, length);

        if (r > 0) {
            //check if it's the same data
            for (int i = 0; i < length; i++) {
                if (prev_data[i] != data[i]) {
                    same = FALSE;
                }
            }
        }
        if (!same) printf("Flushed Packet\n");
        new_packet = !same;
    }

    return 0;
}

///////////////// END USB CODE //////////////////////////

club getClubData(int club_num) {
    club current_club;

    switch (club_num) {
    case Driver:
        current_club.club_mass = 180;
        current_club.club_loft = 10;
        current_club.launch_angle = 10.9;
        current_club.club_speed = 113;
        current_club.smash_factor = 1.48;
        break;
    case W3:
        current_club.club_mass = 190;
        current_club.club_loft = 16.5;
        current_club.launch_angle = 9.2;
        current_club.club_speed = 107;
        current_club.smash_factor = 1.48;
        break;
    case W5:
        current_club.club_mass = 200;
        current_club.club_loft = 21;
        current_club.launch_angle = 9.4;
        current_club.club_speed = 103;
        current_club.smash_factor = 1.47;
        break;
    case HY:
        current_club.club_mass = 195;
        current_club.club_loft = 19;
        current_club.launch_angle = 10.2;
        current_club.club_speed = 100;
        current_club.smash_factor = 1.46;
        break;
    case I3:
        current_club.club_mass = 230;
        current_club.club_loft = 19;
        current_club.launch_angle = 10.4;
        current_club.club_speed = 98;
        current_club.smash_factor = 1.45;
        break;
    case I4:
        current_club.club_mass = 237;
        current_club.club_loft = 21;
        current_club.launch_angle = 11.0;
        current_club.club_speed = 96;
        current_club.smash_factor = 1.43;
        break;
    case I5:
        current_club.club_mass = 244;
        current_club.club_loft = 23.5;
        current_club.launch_angle = 12.1;
        current_club.club_speed = 94;
        current_club.smash_factor = 1.41;
        break;
    case I6:
        current_club.club_mass = 251;
        current_club.club_loft = 26.5;
        current_club.launch_angle = 14.1;
        current_club.club_speed = 92;
        current_club.smash_factor = 1.38;
        break;
    case I7:
        current_club.club_mass = 258;
        current_club.club_loft = 30.5;
        current_club.launch_angle = 16.3;
        current_club.club_speed = 90;
        current_club.smash_factor = 1.33;
        break;
    case I8:
        current_club.club_mass = 265;
        current_club.club_loft = 34.5;
        current_club.launch_angle = 18.1;
        current_club.club_speed = 87;
        current_club.smash_factor = 1.32;
        break;
    case I9:
        current_club.club_mass = 270;
        current_club.club_loft = 38.5;
        current_club.launch_angle = 20.4;
        current_club.club_speed = 85;
        current_club.smash_factor = 1.28;
        break;
    case PW:
        current_club.club_mass = 300;
        current_club.club_loft = 43;
        current_club.launch_angle = 24.2;
        current_club.club_speed = 83;
        current_club.smash_factor = 1.23;
        break;
    case GW:
        current_club.club_mass = 300;
        current_club.club_loft = 48;
        current_club.launch_angle = 26;
        current_club.club_speed = 81;
        current_club.smash_factor = 1.21;
        break;
    case SW:
        current_club.club_mass = 300;
        current_club.club_loft = 54;
        current_club.launch_angle = 30;
        current_club.club_speed = 79;
        current_club.smash_factor = 1.19;
        break;
    case Putter:
        current_club.club_mass = 140;
        current_club.club_loft = 0;
        current_club.launch_angle = 0;
        current_club.club_speed = 10;
        current_club.smash_factor = 1.0;
        break;
    default:
        current_club.club_mass = 140;
        current_club.club_loft = 10;
        current_club.launch_angle = 10.9;
        current_club.club_speed = 114;
        current_club.smash_factor = 1.48;
        break;
    }

    return current_club;
}

void processShotData(uint8_t* data, int data_size) {

    // FIGURE OUT DATA PROCESSING
    bool first_front = TRUE;
    int elapsed_time = 0;
    bool potential_ball_read = FALSE;
    bool no_ball = FALSE;
    double swing_speed = 0.0;
    int min_front = 7;
    int max_front = 0;
    int min_back = 7;
    int max_back = 0;
    double front_average = 0.0;
    int front_count = 0;
    double back_average = 0.0;
    int back_count = 0;
    int ball_skip = 0;

    for (int i = 0; i < data_size; i += 5) {
        if (data[i + 2] == 0x81) {
            printf("Origin:     Sensor(s) ");
            if (data[i] != 0) {
                for (int j = 0; j < 8; j++) {
                    if (((data[i] >> j) & 0x01) != 0) {
                        if (j < min_front) min_front = j;
                        if (j > max_front) max_front = j;
                        front_average += j;
                        front_count++;
                    }
                    if (((data[i] >> j) & 0x01) != 0) printf("F%d, ", j);
                    else printf("    ");
                }
            }
            else {
                for (int j = 0; j < 8; j++) {
                    if (((data[i + 1] >> j) & 0x01) != 0) {
                        if (j < min_back) min_back = j;
                        if (j > max_back) max_back = j;
                        back_average += j;
                        back_count++;
                    }
                    if (((data[i + 1] >> j) & 0x01) != 0) printf("B%d, ", j);
                    else printf("    ");
                }
            }
            printf("\n");
        }
        if (data[i + 2] == 0x52) {
            elapsed_time += ((data[i + 3] * 256) + data[i + 4]);
            printf("Additional: Sensor(s) ");
            for (int j = 0; j < 8; j++) {
                if (((data[i] >> j) & 0x01) != 0) printf("ERROR IN UNDERSTANDING ");
            }
            for (int j = 0; j < 8; j++) {
                if (((data[i + 1] >> j) & 0x01) != 0) {
                    if (j < min_back) min_back = j;
                    if (j > max_back) max_back = j;
                    back_average += j;
                    back_count++;
                }
                if (((data[i + 1] >> j) & 0x01) != 0) printf("B%d, ", j);
                else printf("    ");
            }
            printf("\n");
        }
        if (data[i + 2] == 0x4A) {
            elapsed_time += ((data[i + 3] * 256) + data[i + 4]);
            printf("Additional: Sensor(s) ");
            for (int j = 0; j < 8; j++) {
                if (((data[i] >> j) & 0x01) != 0) {
                    if (j < min_front) min_front = j;
                    if (j > max_front) max_front = j;
                    front_average += j;
                    front_count++;
                }
                if (((data[i] >> j) & 0x01) != 0) printf("F%d, ", j);
                else printf("    ");
            }
            for (int j = 0; j < 8; j++) {
                if (((data[i + 1] >> j) & 0x01) != 0) printf("ERROR IN UNDERSTANDING ");
            }
            if (first_front) {
                first_front = FALSE;
                //printf("Elapsed: %d, Gap: %d\n", elapsed_time, ((data[i + 3] * 256) + data[i + 4]));
                swing_speed = (double(SENSORSPACING) / double(elapsed_time * 18)) * 2236.94;
            }
            else if (usingball) {
                if (!no_ball) {
                    if (potential_ball_read) {
                        if (((data[i + 3] * 256) + data[i + 4]) < 0x20) {
                            swing_speed = (double(SENSORSPACING) / (double(elapsed_time - ((data[i - 2] * 256) + data[i - 1])) * 18)) * 2236.94;
                            ball_skip = i - 5;
                            printf("Ball detected, swing speed updated!\n");
                        }
                        else {
                            no_ball = TRUE;
                        }
                    }
                    if (((data[i + 3] * 256) + data[i + 4]) > 0x25) {
                        potential_ball_read = TRUE;
                        printf("Maybe a ball?\n");
                    }
                }
            }
            printf("\n");
        }
    }

    //data hex printout
    for (int i = 0; i < data_size; i++) {
        if (i % 20 == 0 && i != 0) printf("\n");
        else if (i % 5 == 0 && i != 0) printf("  ");
        printf("%02hhx ", data[i]);
        if (logging) fprintf(fp, "%d,", data[i]);
    }
    printf("\n\n");
    if (logging) fprintf(fp, "\n");


    std::wstringstream clubspeed;
    //Swing Speed MPH
    if (!first_front) clubspeed << std::fixed << std::setw(3) << std::setprecision(1) << swing_speed << " MPH\n";
    SetWindowText(clubSpeedValue, clubspeed.str().c_str());

    //Club Open/Closed
    int prev_min, prev_max, min, max;
    double min_angle, max_angle;
    double back_angle_accumulate = 0.0;
    int back_angle_count = 0;
    double front_angle_accumulate = 0.0;
    int front_angle_count = 0;
    bool front = FALSE;
    double speed_per_tick = double(SENSORSPACING) / double(elapsed_time);
    for (int i = 0; i < data_size; i += 5) {
        if (i == 0) {
            int temp_data = 0;
            if (data[i] == 0) temp_data = data[i + 1];
            else temp_data = data[i];
            int j = 0;
            while (((temp_data >> j) & 0x1) == 0) j++;
            prev_min = j;
            j = 7;
            while (((temp_data >> j) & 0x1) == 0) j--;
            prev_max = j;
        }
        else {
            if (i >= ball_skip) {
                if (data[i] != 0 && !front) {
                    int temp_data = 0;
                    if (data[i] == 0) temp_data = data[i + 1];
                    else temp_data = data[i];
                    int j = 0;
                    while (((temp_data >> j) & 0x1) == 0) j++;
                    prev_min = j;
                    j = 7;
                    while (((temp_data >> j) & 0x1) == 0) j--;
                    prev_max = j;
                    front = TRUE;
                }
                else {
                    if (data[i] != 00 || data[i + 1] != 00) {
                        int temp_data = 0;
                        if (data[i] == 0) temp_data = data[i + 1];
                        else temp_data = data[i];
                        int j = 0;
                        while (((temp_data >> j) & 0x1) == 0) j++;
                        min = j;
                        j = 7;
                        while (((temp_data >> j) & 0x1) == 0) j--;
                        max = j;
                        double x_travel = speed_per_tick * ((data[i + 3] * 256) + data[i + 4]);
                        int y_travel_min = (prev_min - min) * LEDSPACING;
                        if (y_travel_min == 0) y_travel_min = 1000000;
                        int y_travel_max = (prev_max - max) * LEDSPACING;
                        if (y_travel_max == 0) y_travel_max = 1000000;
                        //printf("x: %f, Ymin: %d, Ymax: %d\n", x_travel, y_travel_min, y_travel_max);
                        min_angle = atan(x_travel / y_travel_min) * 180 / PI;
                        max_angle = atan(x_travel / y_travel_max) * 180 / PI;

                        if (front) {
                            //printf("Min: %f, Max %f\n", min_angle, max_angle);
                            if (abs(min_angle) > abs(max_angle)) front_angle_accumulate += min_angle;
                            else front_angle_accumulate += max_angle;
                            front_angle_count++;
                        }
                        else {
                            //printf("Min: %f, Max %f\n", min_angle, max_angle);
                            if (abs(min_angle) > abs(max_angle)) back_angle_accumulate += min_angle;
                            else back_angle_accumulate += max_angle;
                            back_angle_count++;
                        }
                        prev_min = min;
                        prev_max = max;
                    }
                }
            }
        }
    }
    //printf("%f, %f, %d, %d\n", front_angle_accumulate, back_angle_accumulate, front_angle_count, back_angle_count);
    double average = ((front_angle_accumulate / front_angle_count) + (back_angle_accumulate / back_angle_count) * 2) / 3; //weighted as the ball is closer to the back sensor
    if (isnan(average)) average = 0.0;
    //printf("Front: %f, Back %f\n", front_angle_accumulate / front_angle_count, back_angle_accumulate / back_angle_count);
    //wss << "Face Angle: ";
    std::wstringstream faceangle;
    faceangle << std::fixed << std::setw(3) << std::setprecision(1);
    if (lefty) {
        if (average > 0) faceangle << "Open " << average << "\n";
        else if (average < 0) faceangle << "Closed " << abs(average) << "\n";
        else faceangle << "Square " << abs(average) << "\n";
    }
    else {
        if (average > 0) faceangle << "Closed " << average << "\n";
        else if (average < 0) faceangle << "Open " << abs(average) << "\n";
        else faceangle << "Square " << abs(average) << "\n";
    }
    SetWindowText(faceAngleValue, faceangle.str().c_str());
    swing_angle = average;

    //Club Path
    int max_path = max_front - max_back;
    int min_path = min_front - min_back;
    int path = max_path + min_path;
    //wss << "max: " << max_path << " min: " << min_path << " path: " << path << "\n";
    //printf("%d ", path);
    //wss << "Path: ";
    std::wstringstream clubpath;
    if (lefty) {
        if (abs(path) > 3) {
            if (path > 0) clubpath << "Very Outside/In\n";
            else clubpath << "Very Inside/Out\n";
        }
        else if (abs(path) > 1) {
            if (path > 0) clubpath << "Outside/In\n";
            else clubpath << "Inside/Out\n";
        }
        else clubpath << "On Plane\n";
    }
    else {
        if (abs(path) > 3) {
            if (path > 0) clubpath << "Very Inside/Out\n";
            else clubpath << "Very Outside/In\n";
        }
        else if (abs(path) > 1) {
            if (path > 0) clubpath << "Inside/Out\n";
            else clubpath << "Outside/In\n";
        }
        else clubpath << "On Plane\n";
    }
    SetWindowText(pathValue, clubpath.str().c_str());
    swing_path = path;

    //Club Face Hit Location
    //wss << "Face Contact: ";
    std::wstringstream facecontact;
    //Only using back, matches OS software better, and makes more sense as the front is way after the ball
    int max_trigger = max_back; // max(max_front, max_back);
    int min_trigger = min_back; // max(min_front, min_back);
    if (lefty) {
        if (min_trigger == 7) { facecontact << "Missed\n"; face_contact = 3; }
        else if (min_trigger == 6) { facecontact << "Extreme Toe\n";  face_contact = 2; }
        else if (min_trigger == 5) { facecontact << "Extreme Toe\n";  face_contact = 2; }
        else if (min_trigger == 4) { facecontact << "Toe\n";          face_contact = 1; }
        else if (max_trigger == 0) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (max_trigger == 1) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (max_trigger == 2) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (max_trigger == 3) { facecontact << "Heel\n";         face_contact = -1; }
        else { facecontact << "Center\n"; face_contact = 0; }
    }
    else {
        if (max_trigger == 0) { facecontact << "Missed\n"; face_contact = 3; }
        else if (max_trigger == 1) { facecontact << "Extreme Toe\n";  face_contact = 2; }
        else if (max_trigger == 2) { facecontact << "Extreme Toe\n";  face_contact = 2; }
        else if (max_trigger == 3) { facecontact << "Toe\n";          face_contact = 1; }
        else if (min_trigger == 7) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (min_trigger == 6) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (min_trigger == 5) { facecontact << "Far Heel\n";     face_contact = -2; }
        else if (min_trigger == 4) { facecontact << "Heel\n";         face_contact = -1; }
        else { facecontact << "Center\n"; face_contact = 0; }
    }
    SetWindowText(faceContactValue, facecontact.str().c_str());

    //wss << selected_club << "\n";
    club current_club = getClubData(selected_club);

    if (max_trigger == 0) current_club.smash_factor = 0.5;
    else if (max_trigger == 1) current_club.smash_factor = 0.94;
    else if (max_trigger == 2) current_club.smash_factor = 0.94;
    else if (max_trigger == 3) current_club.smash_factor = 0.98;
    else if (min_trigger == 7) current_club.smash_factor = 0.94;
    else if (min_trigger == 6) current_club.smash_factor = 0.94;
    else if (min_trigger == 5) current_club.smash_factor = 0.94;
    else if (min_trigger == 4) current_club.smash_factor = 0.98;

    //Generate Parameters for shot
    if (selected_club == Putter) {
        backswingstepsize = 7;
        forwardswingstepsize = 25;
        if (swing_speed > 5.5) midswingdelay = 100 + int(1500 * (swing_speed / current_club.club_speed));
        else midswingdelay = 100 + int(1000 * (swing_speed / current_club.club_speed));
    }
    else
    {
        backswingstepsize = 7.0 * current_club.smash_factor;
        forwardswingstepsize = 25.0 * current_club.smash_factor; // 15/50 goes right, 150/7 goes left, no control beyond default

        if (selected_club > I9) { // Wedges
            midswingdelay = 100 + int(500.0 * (swing_speed / current_club.club_speed));
        }
        else if (selected_club > W5) { // Hybrid and Irons
            midswingdelay = 50 + int(575.0 * (swing_speed / current_club.club_speed));
        }
        else { // Driver and Woods
            midswingdelay = 100 + int(525.0 * (swing_speed / current_club.club_speed));
        }
    }
    slope = path * .08333333; //affects ball trajectory (point left/right for shot), unfortunately Opti has rough granulatiry

    double off_angle = (atan(slope) * -180 / PI) - average;

    slope += tan(off_angle* PI / 180) / 5; //ball launch adjustment based on how open/closed the club is relative to the path
    slope *= 2.5; //scaling as TGC has a "deadzone"

    sideaccel = off_angle; //only control on swing is how fast the club goes forward
    sidescale = 0;         //can control by holding shift and pressing left/right
    
    RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void preciseSleep(double seconds) {
    using namespace std;
    using namespace std::chrono;

    static double estimate = 5e-3;
    static double mean = 5e-3;
    static double m2 = 0;
    static int64_t count = 1;

    while (seconds > estimate) {
        auto start = high_resolution_clock::now();
        this_thread::sleep_for(milliseconds(1));
        auto end = high_resolution_clock::now();

        double observed = (end - start).count() / 1e9;
        seconds -= observed;

        ++count;
        double delta = observed - mean;
        mean += delta / count;
        m2 += delta * (observed - mean);
        double stddev = sqrt(m2 / (count - 1));
        estimate = mean + stddev;
    }

    // spin lock
    auto start = high_resolution_clock::now();
    while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
}

void setCurve()
{
    INPUT inputs[2] = {};
    ZeroMemory(inputs, sizeof(inputs));

    inputs[0].type = INPUT_KEYBOARD; //tap W, sometimes the first keyboard input is missed, so this one is throw away
    inputs[0].ki.wVk = 0x57;
    inputs[0].ki.dwFlags = 0;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 0x57;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    Sleep(10);

    inputs[0].type = INPUT_KEYBOARD; //tap Z to change club
    inputs[0].ki.wVk = 0x5A;
    inputs[0].ki.dwFlags = 0;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 0x5A;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    Sleep(10);

    inputs[0].type = INPUT_KEYBOARD; //tap x to go back to same club
    inputs[0].ki.wVk = 0x58;         //this resets the shot curve to be straight
    inputs[0].ki.dwFlags = 0;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 0x58;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    Sleep(10);

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LSHIFT;
    inputs[0].ki.dwFlags = 0;

    if (sideaccel > 0) {
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_RIGHT;
        inputs[1].ki.dwFlags = 0 | KEYEVENTF_EXTENDEDKEY;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        preciseSleep(abs(sideaccel) / 25.0);

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_RIGHT;
        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
    }
    else if(sideaccel < 0) {
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_LEFT;
        inputs[1].ki.dwFlags = 0 | KEYEVENTF_EXTENDEDKEY;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
        preciseSleep(abs(sideaccel) / 25.0);

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_LEFT;
        inputs[0].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
    }

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_LSHIFT;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    Sleep(10);
}

void takeShot() {
    double cursorX = desktop_width/2;
    double cursorY = desktop_height/2;
    int backswingspeed = 6;
    int forwardswingspeed = 5; //lower is faster
    //double local_sideaccel = sideaccel;

    SetCursorPos(cursorX, cursorY);
    if (clickMouse) {
        mouse_event(MOUSEEVENTF_LEFTDOWN, cursorX, cursorY, 0, 0); //click the window to ensure it's in focus before doing keyboard stuff
        Sleep(100);
        mouse_event(MOUSEEVENTF_LEFTUP, cursorX, cursorY, 0, 0);
        Sleep(100);
        if(selected_club != Putter) setCurve();
        mouse_event(MOUSEEVENTF_LEFTDOWN, cursorX, cursorY, 0, 0); //click for start of shot
    }
    for (int i = 0; i < backswingsteps; i++) {
        cursorY += backswingstepsize;
        cursorX -= slope*backswingstepsize;
        if (cursorX < 0) cursorX = 0;
        if (cursorX > desktop_width) cursorX = desktop_width;
        if (cursorY < 0) cursorY = 0;
        if (cursorY > desktop_height) cursorY = desktop_height;
        SetCursorPos(cursorX, cursorY);
        preciseSleep(double(backswingspeed)/1000.0);
    }
    preciseSleep(double(midswingdelay)/1000.0); //convert ms to s
    for (int i = 0; i < forwardswingsteps; i++) {
        cursorY -= forwardswingstepsize;
        cursorX += slope*forwardswingstepsize;
        //local_sideaccel = pow(local_sideaccel, 1.2);
        //if (local_sideaccel > desktop_width) local_sideaccel = desktop_width;
        //cursorX += int(local_sideaccel * sidescale);
        if (cursorX < 0) cursorX = 0;
        if (cursorX > desktop_width) cursorX = desktop_width;
        if (cursorY < 0) cursorY = 0;
        if (cursorY > desktop_height) cursorY = desktop_height;
        SetCursorPos(cursorX, cursorY);
        preciseSleep(double(forwardswingspeed)/1000.0);
    }
    mouse_event(MOUSEEVENTF_LEFTUP, cursorX, cursorY, 0, 0);
    SetCursorPos(cursorX, cursorY);
}

void optiPolling(hid_device* dev, libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer) {
    int r, i;
    int size = 60;
    int data_size = 60;
    uint8_t* data;
    data = (uint8_t*)calloc(size, 1);
    uint8_t* prev_data;
    prev_data = (uint8_t*)calloc(size, 1);
    for (i = 0; i < data_size; i++) {
        data[i] = 0;
        prev_data[i] = 0;
    }

    while (keep_polling) {
        r = 0;
        r = hid_read(dev, data, data_size);
        Sleep(100);

        if (r > 0) {
            //check if it's the same data
            bool same = TRUE;
            for (i = 0; i < data_size; i++) {
                if (prev_data[i] != data[i]) {
                    same = FALSE;
                }
            }

            if (!same) {

                if (collect_swing) {
                    bool back_orig = FALSE;
                    bool front = FALSE;

                    //Check for a proper swing (data packet has both back and front sensor data)
                    for (int i = 0; i < data_size; i += 5) {
                        if (data[i + 2] == 0x81) {
                            if (data[i] == 0) {
                                back_orig = TRUE;
                            }
                        }
                        if (data[i + 2] == 0x4A) {
                            front = TRUE;
                        }
                    }

                    if (back_orig && front) {

                        opti_red(handle, endpoint, report_buffer, size);

                        processShotData(data, data_size);

                        takeShot();

                        //TODO: determine sleep time, hopefully just one good delay time will work
                        Sleep(SHOTSLEEPTIME);

                        //flush any extra packets so we don't do back to back shots, etc. then return to valid for user
                        flush_buffer(dev, data, prev_data, data_size);
                        opti_green(handle, endpoint, report_buffer, size);
                    }
                    else 
                    if (front) { //if it's only the front sensor, use it to change the club selection

                        bool increment = FALSE;
                        uint8_t agregate = 0x00;
                        for (int i = 0; i < data_size; i += 5) {
                            if ((data[i] & 0xE0) != 0) {
                                agregate = agregate | data[i];
                                increment = TRUE;
                            }
                        }

                        INPUT inputs[2] = {};
                        ZeroMemory(inputs, sizeof(inputs));

                        inputs[0].type = INPUT_KEYBOARD; //tap W, sometimes the first keyboard input is missed, so this one is throw away
                        inputs[0].ki.wVk = 0x57;
                        inputs[0].ki.dwFlags = 0;

                        inputs[1].type = INPUT_KEYBOARD;
                        inputs[1].ki.wVk = 0x57;
                        inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

                        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
                        Sleep(10);
                        if (agregate == 0xFF) { //full swipe over sensor, change shot shape
                            inputs[0].type = INPUT_KEYBOARD; //tap C to change shot
                            inputs[0].ki.wVk = 0x43;
                            inputs[0].ki.dwFlags = 0;

                            inputs[1].type = INPUT_KEYBOARD;
                            inputs[1].ki.wVk = 0x43;
                            inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

                            SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
                            Sleep(10);
                        }
                        else if (increment) { //also press "Z"
                            if (selected_club > Driver) selected_club--;
                            else selected_club = Putter;
                        }
                        else { //also press "X"
                            if (selected_club < Putter) selected_club++;
                            else selected_club = Driver;
                        }
                        SendMessage(clubSelect, CB_SETCURSEL, (WPARAM)selected_club, (LPARAM)0);
                        Sleep(250);
                        flush_buffer(dev, data, prev_data, data_size);
                    }

                }

                for (i = 0; i < data_size; i++) {
                    prev_data[i] = data[i]; //copy data to prev buffer
                }
            }
        }
        if(on_top) SetWindowPos(mainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

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
    libusb_device_handle* handle = NULL;
    uint8_t endpoint = 1;
    int size = 60;
    uint8_t* report_buffer;
    report_buffer = (uint8_t*)calloc(size, 1);
    hid_device* dev;

    fopen_s(&fp, "data_log.txt", "a+");

    ////////// LIBUSB SETUP //////////
    // Optishot VID:PID
    VID = 0x0547;
    PID = 0x3294;

    r = libusb_init(NULL);
    if (r < 0)
        return r;

    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO); //LIBUSB_LOG_LEVEL_INFO LIBUSB_LOG_LEVEL_DEBUG

    int retry = 10;
    while (handle == NULL && retry == 10) {
        handle = init_device(VID, PID, NULL); //opening data socket and receiving data, I need to get it.
        if(handle == NULL) retry = MessageBox(GetActiveWindow(), L"Cannot Connect to Optishot, ensure the USB cable is plugged in and click Try Again.\n\n Press Cancel to close the application.\n\nPress continue to run without Optishot.", NULL, MB_CANCELTRYCONTINUE);
    }
    if (handle != NULL) {
        ////////// END LIBUSB SETUP ////////

        ///////// OPTI-CONTROL //////////////
        r = opti_init(handle, endpoint, report_buffer, size);
        if (r != 0) return r;

        ////CAPTURE DATA LOOP////////////
        dev = hid_open(VID, PID, NULL);

        hid_set_nonblocking(dev, 1);

        std::thread polling(optiPolling, dev, handle, endpoint, report_buffer);

        ////////////////////////////////////////////////////////// END USB CODE /////////

        // Main message loop:
        while (GetMessage(&msg, nullptr, 0, 0))
        {

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        /////////////////////////////////////////////// USB CODE ////////////

        keep_polling = false;
        polling.join();

        r = opti_shutdown(handle, endpoint, report_buffer, size);
        if (r != 0) return r;
        //////// END OPTI-CONTROL ///////////

        hid_close(dev);
        libusb_close(handle);
    }
    else if (retry == 11) {
        // Main message loop:
        while (GetMessage(&msg, nullptr, 0, 0))
        {

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    libusb_exit(NULL);
    fclose(fp);

    ////////////////////////////////////////////////////////// END USB CODE /////////
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

    clubSelect = CreateWindowW(TEXT("COMBOBOX"), TEXT("Reswing"), WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, 10, y_offset, 100, 200, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    y_offset += 30;

    TCHAR Clubs[15][16] =
    {
        TEXT("Driver"), TEXT("3 Wood"), TEXT("5 Wood"), TEXT("Hybrid"),
        TEXT("3 Iron"), TEXT("4 Iron"), TEXT("5 Iron"), TEXT("6 Iron"), TEXT("7 Iron"), TEXT("8 Iron"), TEXT("9 Iron"),
        TEXT("Pitching Wedge"), TEXT("Gap Wedge"), TEXT("Sand Wedge"), TEXT("Putter")
    };

    TCHAR A[16];
    int  k = 0;

    memset(&A, 0, sizeof(A));
    for (k = 0; k < 15; k += 1)
    {
        wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)Clubs[k]);

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
        }
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
            selected_club = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
            TCHAR  ListItem[256];
            (TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)selected_club, (LPARAM)ListItem);
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
            //clubs
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

        //Draw Golf Ball
        hdcMem = CreateCompatibleDC(hdc);
        oldBitmap = SelectObject(hdcMem, golfball_bmap);

        GetObject(golfball_bmap, sizeof(bitmap), &bitmap);
        BitBlt(hdc, left+135, top+50, left+bitmap.bmWidth, top+bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, oldBitmap);
        DeleteDC(hdcMem);

        //Draw Club
        HBITMAP club_image = NULL;
        if (selected_club < I3) club_image = driver_bmap;
        else if (selected_club == Putter) club_image = putter_bmap;
        else club_image = iron_bmap;

        if (lefty) swing_angle += 180;
        club_image = GetRotatedBitmapNT(hdc, club_image, (swing_angle * PI / 180.0), COLORREF(0x00FFFFFF));

        hdcMem = CreateCompatibleDC(hdc);
        oldBitmap = SelectObject(hdcMem, club_image);
        
        GetObject(club_image, sizeof(bitmap), &bitmap);

        double path_offset = swing_path * .08333;
        if(lefty) BitBlt(hdc, left + 97 - bitmap.bmWidth, top + 25 + int(path_offset * -55) + (face_contact * 17), left + bitmap.bmWidth, top + bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
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


        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        DeleteObject(golfball_bmap);
        DeleteObject(driver_bmap);
        DeleteObject(iron_bmap);
        DeleteObject(putter_bmap);
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
