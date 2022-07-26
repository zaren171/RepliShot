#include "usbcode.h"

// Future versions of libusb will use usb_interface instead of interface
// in libusb_config_descriptor => catter for that
#define usb_interface interface

#define SHOTSLEEPTIME 2500

uint16_t VID, PID;

extern bool collect_swing;
extern bool keep_polling;
extern bool clientmode;
extern bool clientConnected;
extern bool front_sensor_features;
extern bool clickMouse;
extern bool on_top;
extern bool opticonnected;
extern bool club_lockstep;
extern bool club_lockstep;

extern int selected_club;
extern int num_clubs;
extern int selected_club_value;
extern struct Node* clubs;
extern struct Node* current_selected_club;

extern HWND mainWindow;
extern HWND clubSelect;

extern SOCKET HostSocket;

extern uint8_t* report_buffer;
extern hid_device* dev;
extern libusb_device_handle* handle;
extern std::vector<std::thread> threads;

extern std::mutex club_data;

void perr(char const* format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

libusb_device_handle* init_device(uint16_t vid, uint16_t pid, libusb_context* context)
{
    libusb_device_handle* handle;

    printf("Opening device %04X:%04X...\n", vid, pid);
    handle = libusb_open_device_with_vid_pid(context, vid, pid);

    return handle;
}

int usb_init(libusb_device_handle* handle) {
    int r = 0;

    libusb_set_auto_detach_kernel_driver(handle, 1);

    r = libusb_claim_interface(handle, 0);
    if (r != LIBUSB_SUCCESS) {
        perr("   Failed.\n");
    }

    return r;
}

int opti_red(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size) {
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

int opti_green(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size) {
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

int opti_cycle(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size, int sleep_time) {
    int r = 0;

    r = opti_red(handle, endpoint, report_buffer, size);

    Sleep(sleep_time);

    r = opti_green(handle, endpoint, report_buffer, size);

    return 0;
}

int opti_init(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size) {
    int r = 0;

    r = usb_init(handle);
    //if (r != 0) {
    //    return -1;
    //}

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

int opti_shutdown(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size) {
    int r = 0;

    //Turns off sensors/LED
    report_buffer[0] = 0x80;
    r = libusb_interrupt_transfer(handle, endpoint, report_buffer, size, &size, 5000);
    if (r != LIBUSB_SUCCESS) {
        perr("   Failed.\n");
    }

    return 0;
}

int flush_buffer(hid_device* dev, unsigned char* data, unsigned char* prev_data, size_t length) {
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

                        if (clientmode && clientConnected) {
                            send(HostSocket, (const char*)data, data_size, 0);
                            //Sleep(1);
                            //wait for ack
                            //if timeout continue
                        }
                        else {
                            processShotData(data, data_size);

                            takeShot();
                        }

                        //TODO: determine sleep time, hopefully just one good delay time will work
                        Sleep(SHOTSLEEPTIME);

                        //flush any extra packets so we don't do back to back shots, etc. then return to valid for user
                        flush_buffer(dev, data, prev_data, data_size);
                        opti_green(handle, endpoint, report_buffer, size);
                    }
                    else
                        if (front && front_sensor_features) { //if it's only the front sensor, use it to change the club selection

                            bool increment = FALSE;
                            uint8_t agregate = 0x00;
                            for (int i = 0; i < data_size; i += 5) {
                                agregate = agregate | data[i];
                                if ((data[i] & 0xE0) != 0) {
                                    increment = TRUE;
                                }
                            }

                            INPUT inputs[2] = {};
                            ZeroMemory(inputs, sizeof(inputs));

                            club_data.lock();
                            if (clickMouse) {
                                inputs[0].type = INPUT_KEYBOARD; //tap W, sometimes the first keyboard input is missed, so this one is throw away
                                inputs[0].ki.wVk = 0x57;
                                inputs[0].ki.dwFlags = 0;

                                inputs[1].type = INPUT_KEYBOARD;
                                inputs[1].ki.wVk = 0x57;
                                inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

                                SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
                                Sleep(10);
                            }
                            if ((agregate & 0xC3) == 0 && clickMouse) { //middle of sensor, change shot shape
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
                                if (selected_club > 0) {
                                    current_selected_club = current_selected_club->prev;
                                    selected_club_value = current_selected_club->club;
                                    selected_club--;
                                }
                                else {
                                    current_selected_club = clubs->prev;
                                    selected_club_value = current_selected_club->club;
                                    selected_club = num_clubs;
                                }
                                if (club_lockstep && clickMouse) {
                                    inputs[0].type = INPUT_KEYBOARD; //tap X to change club
                                    inputs[0].ki.wVk = 0x58;
                                    inputs[0].ki.dwFlags = 0;

                                    inputs[1].type = INPUT_KEYBOARD;
                                    inputs[1].ki.wVk = 0x58;
                                    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

                                    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
                                    Sleep(10);
                                }
                            }
                            else { //also press "X"
                                if (selected_club < num_clubs) {
                                    current_selected_club = current_selected_club->next;
                                    selected_club_value = current_selected_club->club;
                                    selected_club++;
                                }
                                else {
                                    current_selected_club = clubs;
                                    selected_club_value = current_selected_club->club;
                                    selected_club = 0;
                                }
                                if (club_lockstep && clickMouse) {
                                    inputs[0].type = INPUT_KEYBOARD; //tap Z to change club
                                    inputs[0].ki.wVk = 0x5A;
                                    inputs[0].ki.dwFlags = 0;

                                    inputs[1].type = INPUT_KEYBOARD;
                                    inputs[1].ki.wVk = 0x5A;
                                    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

                                    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
                                    Sleep(10);
                                }
                            }
                            SendMessage(clubSelect, CB_SETCURSEL, (WPARAM)selected_club, (LPARAM)0);
                            club_data.unlock();

                            Sleep(250);
                            flush_buffer(dev, data, prev_data, data_size);
                        }

                }

                for (i = 0; i < data_size; i++) {
                    prev_data[i] = data[i]; //copy data to prev buffer
                }
            }
        }
        else {
            if (libusb_open_device_with_vid_pid(NULL, VID, PID) == NULL) {
                opticonnected = FALSE;
                RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
                return;
            }
        }
        if (on_top) SetWindowPos(mainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    }
}

int opti_connect() {
    int retry = IDRETRY;

    // Optishot VID:PID
    VID = 0x0547;
    PID = 0x3294;

    libusb_init(NULL);

    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO); //LIBUSB_LOG_LEVEL_INFO LIBUSB_LOG_LEVEL_DEBUG

    handle = init_device(VID, PID, NULL); //opening data socket and receiving data, I need to get it.

    while (handle == NULL && retry == IDRETRY) {
        handle = init_device(VID, PID, NULL); //opening data socket and receiving data, I need to get it.
        if (handle == NULL) retry = MessageBox(GetActiveWindow(), L"Cannot Connect to Optishot, ensure the USB cable is plugged in and click Retry.\n\n Press Cancel to continue without the Optishot.", NULL, MB_RETRYCANCEL);
    }
    if (handle != NULL) {
        ///////// OPTI-CONTROL //////////////
        opti_init(handle, 1, report_buffer, 60);

        ////CAPTURE DATA LOOP////////////
        dev = hid_open(VID, PID, NULL);

        hid_set_nonblocking(dev, 1);

        threads.push_back(std::thread(optiPolling, dev, handle, 1, report_buffer));

        opticonnected = TRUE;

        RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    }

    return retry;
}
