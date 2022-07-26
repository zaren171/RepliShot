#include "networkcode.h"
#include "shotprocessing.h"

extern bool hostmode;
extern bool clientmode;
extern bool keep_polling;
extern bool clientConnected;

extern std::string ip_addr;

extern HWND mainWindow;

extern SOCKET ClientSocket;
extern SOCKET HostSocket;

void network_stack() {
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo* ptr = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    while ((hostmode || clientmode) && keep_polling) {
        if (hostmode) {

            int iSendResult;
            char recvbuf[DEFAULT_BUFLEN];
            int recvbuflen = DEFAULT_BUFLEN;

            // Initialize Winsock
            WSAStartup(MAKEWORD(2, 2), &wsaData);

            ZeroMemory(&hints, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = AI_PASSIVE;

            // Resolve the server address and port
            getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);

            // Create a SOCKET for connecting to server
            ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

            // Setup the TCP listening socket
            bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);

            freeaddrinfo(result);

            listen(ListenSocket, SOMAXCONN);

            // Accept a client socket
            ClientSocket = accept(ListenSocket, NULL, NULL);

            // No longer need server socket
            closesocket(ListenSocket);

            // Receive until the peer shuts down the connection
            do {

                iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);

                processShotData((uint8_t*)recvbuf, recvbuflen);
                takeShot();

            } while (iResult > 0 && keep_polling);

            hostmode = FALSE;
        }
        else if (clientmode && !clientConnected) {
            // Initialize Winsock
            WSAStartup(MAKEWORD(2, 2), &wsaData);

            ZeroMemory(&hints, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            // Resolve the server address and port
            iResult = getaddrinfo(ip_addr.c_str(), DEFAULT_PORT, &hints, &result);

            // Attempt to connect to an address until one succeeds
            for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

                // Create a SOCKET for connecting to server
                HostSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

                // Connect to server.
                iResult = connect(HostSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
                if (iResult == SOCKET_ERROR) {
                    closesocket(HostSocket);
                    HostSocket = INVALID_SOCKET;
                    continue;
                }
                break;
            }

            freeaddrinfo(result);

            if (HostSocket != INVALID_SOCKET) {

                clientConnected = TRUE;
                RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

                while (keep_polling && clientmode) {
                    Sleep(1000);
                }
            }
        }
        else {
            Sleep(1000);
        }
    }

    clientConnected = FALSE;

    RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

    shutdown(ClientSocket, SD_SEND);
    closesocket(ClientSocket);
    shutdown(HostSocket, SD_SEND);
    closesocket(HostSocket);
    WSACleanup();

    RedrawWindow(mainWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
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
