// UDPServer.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include <atomic>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8080"
#define DEFAULT_BUFLEN 512

std::atomic<bool> running(true);

void receiveMessages(SOCKET ServerSocket) {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    int iResult;

    while (running) {
        iResult = recvfrom(ServerSocket, recvbuf, recvbuflen, 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (iResult == SOCKET_ERROR) {
            printf("recvfrom failed: %d\n", WSAGetLastError());
            running = false;
            break;
        }

        recvbuf[iResult] = '\0';
        printf("Client: %s\n", recvbuf);
    }
}

void sendMessages(SOCKET ServerSocket, sockaddr_in clientAddr, int clientAddrLen) {
    char sendbuf[DEFAULT_BUFLEN];
    int iResult;

    while (running) {
        fgets(sendbuf, DEFAULT_BUFLEN, stdin);
        iResult = sendto(ServerSocket, sendbuf, (int)strlen(sendbuf), 0, (struct sockaddr*)&clientAddr, clientAddrLen);
        if (iResult == SOCKET_ERROR) {
            printf("sendto failed: %d\n", WSAGetLastError());
            running = false;
            break;
        }
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

    struct addrinfo* result = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ServerSocket = INVALID_SOCKET;
    ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ServerSocket == INVALID_SOCKET) {
        printf("socket failed: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(ServerSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    // Initial receive to get client address
    char initialRecvBuf[DEFAULT_BUFLEN];
    iResult = recvfrom(ServerSocket, initialRecvBuf, DEFAULT_BUFLEN, 0, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (iResult == SOCKET_ERROR) {
        printf("recvfrom failed: %d\n", WSAGetLastError());
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    printf("Client connected. You can start chatting...\n");

    std::thread receiveThread(receiveMessages, ServerSocket);
    std::thread sendThread(sendMessages, ServerSocket, clientAddr, clientAddrLen);

    receiveThread.join();
    sendThread.join();

    closesocket(ServerSocket);
    WSACleanup();

    return 0;
}
