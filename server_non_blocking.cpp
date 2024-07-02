#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8080"
#define DEFAULT_BUFLEN 512

int main() {
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // Create a socket for the server to listen for incoming connections
    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set the socket to non-blocking mode
    u_long mode = 1;
    iResult = ioctlsocket(ListenSocket, FIONBIO, &mode);
    if (iResult != NO_ERROR) {
        printf("ioctlsocket failed with error: %ld\n", iResult);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Bind the socket
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(atoi(DEFAULT_PORT));

    iResult = bind(ListenSocket, (SOCKADDR *)&service, sizeof(service));
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Listen on the socket
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %s...\n", DEFAULT_PORT);

    // Variables for accepting connections and receiving data
    SOCKET AcceptSocket;
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    char recvBuf[DEFAULT_BUFLEN];
    int recvBufLen = DEFAULT_BUFLEN;

    while (true) {
        // Accept a connection (non-blocking)
        AcceptSocket = accept(ListenSocket, (SOCKADDR *)&clientAddr, &clientAddrSize);
        if (AcceptSocket == INVALID_SOCKET) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                printf("accept failed with error: %d\n", WSAGetLastError());
            }
        } else {
            printf("Client connected: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

            // Receive data from the client (non-blocking)
            iResult = recv(AcceptSocket, recvBuf, recvBufLen, 0);
            if (iResult > 0) {
                printf("Bytes received: %d\n", iResult);

                // Echo the received data back to the client
                iResult = send(AcceptSocket, recvBuf, iResult, 0);
                if (iResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                }
            } else if (iResult == 0) {
                printf("Connection closed by client.\n");
            } else {
                printf("recv failed with error: %d\n", WSAGetLastError());
            }

            // Close the socket for this client
            closesocket(AcceptSocket);
        }

        // Simulate some other processing
        Sleep(1000);
    }

    // Cleanup
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}
