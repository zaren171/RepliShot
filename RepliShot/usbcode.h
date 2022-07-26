#pragma once
#include "customtypes.h"

#include "hidapi.h"
#include "libusb.h"
#include <stdio.h>
#include <thread>
#include <vector>

#include "shotprocessing.h"

extern uint16_t VID, PID;

void perr(char const* format, ...);

libusb_device_handle* init_device(uint16_t vid, uint16_t pid, libusb_context* context);

int usb_init(libusb_device_handle* handle);

int opti_red(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size);

int opti_green(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size);

int opti_cycle(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size, int sleep_time);

int opti_init(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size);

int opti_shutdown(libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer, int size);

int flush_buffer(hid_device* dev, unsigned char* data, unsigned char* prev_data, size_t length);

void optiPolling(hid_device* dev, libusb_device_handle* handle, uint8_t endpoint, uint8_t* report_buffer);

int opti_connect();