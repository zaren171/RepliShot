#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>

#include "customtypes.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "24607"

void network_stack();

bool getMyIP(IPv4& myIP);