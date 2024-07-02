// UDPClient.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include <atomic>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8080"
#define DEFAULT_BUFLEN 512

std::atomic<bool> running(true);

void receiveMessages(SOCKET ClientSocket, struct sockaddr_in serverAddr, int serverAddrLen) {
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;

    while (running) {
        iResult = recvfrom(ClientSocket, recvbuf, recvbuflen, 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
        if (iResult == SOCKET_ERROR) {
            printf("recvfrom failed: %d\n", WSAGetLastError());
            running = false;
            break;
        }

        recvbuf[iResult] = '\0';
        printf("Server: %s\n", recvbuf);
    }
}

void sendMessages(SOCKET ClientSocket, struct sockaddr_in serverAddr, int serverAddrLen) {
    char sendbuf[DEFAULT_BUFLEN];
    int iResult;

    while (running) {
        fgets(sendbuf, DEFAULT_BUFLEN, stdin);
        iResult = sendto(ClientSocket, sendbuf, (int)strlen(sendbuf), 0, (struct sockaddr*)&serverAddr, serverAddrLen);
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

    struct addrinfo* result = NULL, * ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ClientSocket = INVALID_SOCKET;
    ClientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ClientSocket == INVALID_SOCKET) {
        printf("socket failed: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    int serverAddrLen = sizeof(serverAddr);
    memcpy(&serverAddr, result->ai_addr, result->ai_addrlen);

    freeaddrinfo(result);

    // Initial send to connect to server
    const char* initialSendBuf = "Hello from client";
    iResult = sendto(ClientSocket, initialSendBuf, (int)strlen(initialSendBuf), 0, (struct sockaddr*)&serverAddr, serverAddrLen);
    if (iResult == SOCKET_ERROR) {
        printf("sendto failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    printf("Connected to server. You can start chatting...\n");

    std::thread receiveThread(receiveMessages, ClientSocket, serverAddr, serverAddrLen);
    std::thread sendThread(sendMessages, ClientSocket, serverAddr, serverAddrLen);

    receiveThread.join();
    sendThread.join();

    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}
