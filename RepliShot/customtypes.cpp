#include "customtypes.h"


void insertNode(struct Node** start, int value)
{
    // If the list is empty, create a single node
    // circular and doubly list
    if (*start == NULL)
    {
        struct Node* new_node = new Node;
        new_node->club = value;
        new_node->next = new_node->prev = new_node;
        *start = new_node;
        return;
    }

    // If list is not empty

    /* Find last node */
    Node* last = (*start)->prev;

    // Create Node dynamically
    struct Node* new_node = new Node;
    new_node->club = value;

    // Start is going to be next of new_node
    new_node->next = *start;

    // Make new node previous of start
    (*start)->prev = new_node;

    // Make last previous of new node
    new_node->prev = last;

    // Make new node next of old last
    last->next = new_node;
}

club getClubData(int club_num) {
    club current_club;

    switch (club_num) {
    case Driver:
        wcscpy_s((wchar_t*)current_club.name, 16, L"Driver");
        current_club.club_mass = 180;
        current_club.club_loft = 10;
        current_club.launch_angle = 10.9;
        current_club.dist_at_100 = 240;
        current_club.smash_factor = 1.48;
        break;
    case W2:
        wcscpy_s((wchar_t*)current_club.name, 16, L"2 Wood");
        current_club.club_mass = 190;
        current_club.club_loft = 16.5;
        current_club.launch_angle = 9.2;
        current_club.dist_at_100 = 119;
        current_club.smash_factor = 1.48;
        break;
    case W3:
        wcscpy_s((wchar_t*)current_club.name, 16, L"3 Wood");
        current_club.club_mass = 190;
        current_club.club_loft = 16.5;
        current_club.launch_angle = 9.2;
        current_club.dist_at_100 = 209;
        current_club.smash_factor = 1.48;
        break;
    case W4:
        wcscpy_s((wchar_t*)current_club.name, 16, L"4 Wood");
        current_club.club_mass = 190;
        current_club.club_loft = 16.5;
        current_club.launch_angle = 9.2;
        current_club.dist_at_100 = 200;
        current_club.smash_factor = 1.48;
        break;
    case W5:
        wcscpy_s((wchar_t*)current_club.name, 16, L"5 Wood");
        current_club.club_mass = 200;
        current_club.club_loft = 21;
        current_club.launch_angle = 9.4;
        current_club.dist_at_100 = 195;
        current_club.smash_factor = 1.47;
        break;
    case HY2:
        wcscpy_s((wchar_t*)current_club.name, 16, L"2 Hybrid");
        current_club.club_mass = 195;
        current_club.club_loft = 19;
        current_club.launch_angle = 10.2;
        current_club.dist_at_100 = 201;
        current_club.smash_factor = 1.46;
        break;
    case HY3:
        wcscpy_s((wchar_t*)current_club.name, 16, L"3 Hybrid");
        current_club.club_mass = 195;
        current_club.club_loft = 19;
        current_club.launch_angle = 10.2;
        current_club.dist_at_100 = 192;
        current_club.smash_factor = 1.46;
        break;
    case HY4:
        wcscpy_s((wchar_t*)current_club.name, 16, L"4 Hybrid");
        current_club.club_mass = 195;
        current_club.club_loft = 19;
        current_club.launch_angle = 10.2;
        current_club.dist_at_100 = 184;
        current_club.smash_factor = 1.46;
        break;
    case HY5:
        wcscpy_s((wchar_t*)current_club.name, 16, L"5 Hybrid");
        current_club.club_mass = 195;
        current_club.club_loft = 19;
        current_club.launch_angle = 10.2;
        current_club.dist_at_100 = 179;
        current_club.smash_factor = 1.46;
        break;
    case I2:
        wcscpy_s((wchar_t*)current_club.name, 16, L"2 Iron");
        current_club.club_mass = 230;
        current_club.club_loft = 19;
        current_club.launch_angle = 10.4;
        current_club.dist_at_100 = 191;
        current_club.smash_factor = 1.45;
        break;
    case I3:
        wcscpy_s((wchar_t*)current_club.name, 16, L"3 Iron");
        current_club.club_mass = 230;
        current_club.club_loft = 19;
        current_club.launch_angle = 10.4;
        current_club.dist_at_100 = 181;
        current_club.smash_factor = 1.45;
        break;
    case I4:
        wcscpy_s((wchar_t*)current_club.name, 16, L"4 Iron");
        current_club.club_mass = 237;
        current_club.club_loft = 21;
        current_club.launch_angle = 11.0;
        current_club.dist_at_100 = 171;
        current_club.smash_factor = 1.43;
        break;
    case I5:
        wcscpy_s((wchar_t*)current_club.name, 16, L"5 Iron");
        current_club.club_mass = 244;
        current_club.club_loft = 23.5;
        current_club.launch_angle = 12.1;
        current_club.dist_at_100 = 162;
        current_club.smash_factor = 1.41;
        break;
    case I6:
        wcscpy_s((wchar_t*)current_club.name, 16, L"6 Iron");
        current_club.club_mass = 251;
        current_club.club_loft = 26.5;
        current_club.launch_angle = 14.1;
        current_club.dist_at_100 = 154;
        current_club.smash_factor = 1.38;
        break;
    case I7:
        wcscpy_s((wchar_t*)current_club.name, 16, L"7 Iron");
        current_club.club_mass = 258;
        current_club.club_loft = 30.5;
        current_club.launch_angle = 16.3;
        current_club.dist_at_100 = 146;
        current_club.smash_factor = 1.33;
        break;
    case I8:
        wcscpy_s((wchar_t*)current_club.name, 16, L"8 Iron");
        current_club.club_mass = 265;
        current_club.club_loft = 34.5;
        current_club.launch_angle = 18.1;
        current_club.dist_at_100 = 137;
        current_club.smash_factor = 1.32;
        break;
    case I9:
        wcscpy_s((wchar_t*)current_club.name, 16, L"9 Iron");
        current_club.club_mass = 270;
        current_club.club_loft = 38.5;
        current_club.launch_angle = 20.4;
        current_club.dist_at_100 = 128;
        current_club.smash_factor = 1.28;
        break;
    case PW:
        wcscpy_s((wchar_t*)current_club.name, 16, L"Pitching Wedge");
        current_club.club_mass = 300;
        current_club.club_loft = 43;
        current_club.launch_angle = 24.2;
        current_club.dist_at_100 = 118;
        current_club.smash_factor = 1.23;
        break;
    case GW:
        wcscpy_s((wchar_t*)current_club.name, 16, L"Gap Wedge");
        current_club.club_mass = 300;
        current_club.club_loft = 48;
        current_club.launch_angle = 26;
        current_club.dist_at_100 = 108;
        current_club.smash_factor = 1.21;
        break;
    case SW:
        wcscpy_s((wchar_t*)current_club.name, 16, L"Sand Wedge");
        current_club.club_mass = 300;
        current_club.club_loft = 54;
        current_club.launch_angle = 30;
        current_club.dist_at_100 = 97;
        current_club.smash_factor = 1.19;
        break;
    case LW:
        wcscpy_s((wchar_t*)current_club.name, 16, L"Lob Wedge");
        current_club.club_mass = 300;
        current_club.club_loft = 54;
        current_club.launch_angle = 30;
        current_club.dist_at_100 = 86;
        current_club.smash_factor = 1.19;
        break;
    case Putter:
        wcscpy_s((wchar_t*)current_club.name, 16, L"Putter");
        current_club.club_mass = 140;
        current_club.club_loft = 0;
        current_club.launch_angle = 0;
        current_club.dist_at_100 = 10;
        current_club.smash_factor = 1.0;
        break;
    default:
        current_club.club_mass = 140;
        current_club.club_loft = 10;
        current_club.launch_angle = 10.9;
        current_club.dist_at_100 = 240;
        current_club.smash_factor = 1.48;
        break;
    }

    return current_club;
}

bool getMyIP(IPv4& myIP)
{
    char szBuffer[1024];

#ifdef WIN32
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 0);
    if (::WSAStartup(wVersionRequested, &wsaData) != 0)
        return false;
#endif


    if (gethostname(szBuffer, sizeof(szBuffer)) == SOCKET_ERROR)
    {
#ifdef WIN32
        WSACleanup();
#endif
        return false;
    }


    struct hostent* host = gethostbyname(szBuffer);
    if (host == NULL)
    {
#ifdef WIN32
        WSACleanup();
#endif
        return false;
    }

    //Obtain the computer's IP
    myIP.b1 = ((struct in_addr*)(host->h_addr))->S_un.S_un_b.s_b1;
    myIP.b2 = ((struct in_addr*)(host->h_addr))->S_un.S_un_b.s_b2;
    myIP.b3 = ((struct in_addr*)(host->h_addr))->S_un.S_un_b.s_b3;
    myIP.b4 = ((struct in_addr*)(host->h_addr))->S_un.S_un_b.s_b4;

#ifdef WIN32
    WSACleanup();
#endif
    return true;
}