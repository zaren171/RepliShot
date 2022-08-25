#pragma once
#ifndef CUSTOMTYPES
#define CUSTOMTYPES

#include <Windows.h>

#define PI 3.14159265

enum golf_club { Driver, W2, W3, W4, W5, HY2, HY3, HY4, HY5, I2, I3, I4, I5, I6, I7, I8, I9, PW, GW, SW, LW, Putter };

struct club {
    TCHAR name[16];
    double club_mass;
    double club_loft;
    double launch_angle;
    double dist_at_100;
    double club_speed;
    double smash_factor;
};

struct Node
{
    int club;
    struct Node* next; // Pointer to next node
    struct Node* prev; // Pointer to previous node
};

struct IPv4
{
    unsigned char b1, b2, b3, b4;
};

void insertNode(struct Node** start, int value);

club getClubData(int club_num);

bool getMyIP(IPv4& myIP);

#endif
