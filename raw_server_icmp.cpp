// RawSocketServer.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512

void receivePackets(SOCKET rawSocket) {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;

    while (true) {
        iResult = recv(rawSocket, recvbuf, recvbuflen, 0);
        if (iResult == SOCKET_ERROR) {
            printf("recv failed: %d\n", WSAGetLastError());
            break;
        }

        printf("Received %d bytes\n", iResult);
        // Process packet here
    }
}

int main() {
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    SOCKET rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (rawSocket == INVALID_SOCKET) {
        printf("socket failed: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    std::thread receiveThread(receivePackets, rawSocket);

    receiveThread.join();

    closesocket(rawSocket);
    WSACleanup();

    return 0;
}
