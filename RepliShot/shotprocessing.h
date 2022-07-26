#pragma once

#include "customtypes.h"

#include <Windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

#define SENSORSPACING 185
#define LEDSPACING 15

club getClubData(int club_num);

void processShotData(uint8_t* data, int data_size);

void preciseSleep(double seconds);

void setCurve();

void takeShot();