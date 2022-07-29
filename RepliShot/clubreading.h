#pragma once

#include "clubreadingdata.h"

#include <Windows.h>
#include <windef.h>
#include <wingdi.h>
#include <WinUser.h>
#include <math.h>
#include <sstream>
#include <mutex>

void ReadFromScreen(RECT rc);

void clubReading();