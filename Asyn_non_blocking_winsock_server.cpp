#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

#define PORT "8080"
#define BUFFER_SIZE 1024

void HandleClient(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            send(clientSocket, buffer, bytesReceived, 0);
        }
        else if (bytesReceived == 0) {
            std::cout << "Client disconnected: " << clientSocket << std::endl;
            closesocket(clientSocket);
            break;
        }
        else {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket;
    struct addrinfo hints, * result;
    std::vector<WSAEVENT> eventArray;
    std::vector<SOCKET> socketArray;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Create a socket
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (bind(serverSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    WSAEVENT acceptEvent = WSACreateEvent();
    if (WSAEventSelect(serverSocket, acceptEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR) {
        std::cerr << "WSAEventSelect failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    eventArray.push_back(acceptEvent);
    socketArray.push_back(serverSocket);

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        DWORD eventIndex = WSAWaitForMultipleEvents(eventArray.size(), eventArray.data(), FALSE, WSA_INFINITE, FALSE);
        if (eventIndex == WSA_WAIT_FAILED) {
            std::cerr << "WSAWaitForMultipleEvents failed: " << WSAGetLastError() << std::endl;
            break;
        }

        eventIndex -= WSA_WAIT_EVENT_0;
        WSAResetEvent(eventArray[eventIndex]);

        if (socketArray[eventIndex] == serverSocket) {
            SOCKET clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                break;
            }

            WSAEVENT clientEvent = WSACreateEvent();
            if (WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR) {
                std::cerr << "WSAEventSelect failed: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                break;
            }

            eventArray.push_back(clientEvent);
            socketArray.push_back(clientSocket);

            std::cout << "Client connected: " << clientSocket << std::endl;
        }
        else {
            SOCKET clientSocket = socketArray[eventIndex];
            WSANETWORKEVENTS networkEvents;
            WSAEnumNetworkEvents(clientSocket, eventArray[eventIndex], &networkEvents);

            if (networkEvents.lNetworkEvents & FD_READ) {
                HandleClient(clientSocket);
            }

            if (networkEvents.lNetworkEvents & FD_CLOSE) {
                std::cout << "Client disconnected: " << clientSocket << std::endl;
                closesocket(clientSocket);
                WSACloseEvent(eventArray[eventIndex]);
                eventArray.erase(eventArray.begin() + eventIndex);
                socketArray.erase(socketArray.begin() + eventIndex);
            }
        }
    }

    for (auto event : eventArray) {
        WSACloseEvent(event);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
