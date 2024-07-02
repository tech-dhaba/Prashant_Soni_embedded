#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT "8080"
#define BUFFER_SIZE 1024

void receiveMessages(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << buffer << std::endl;
        }
        else if (bytesReceived == 0) {
            std::cout << "Server disconnected." << std::endl;
            break;
        }
        else {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
                break;
            }
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct addrinfo hints, * result;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(SERVER_IP, PORT, &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (connect(clientSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    u_long mode = 1; // 1 to enable non-blocking socket
    ioctlsocket(clientSocket, FIONBIO, &mode);

    std::cout << "Connected to server." << std::endl;

    std::thread recvThread(receiveMessages, clientSocket);
    recvThread.detach();

    char buffer[BUFFER_SIZE];
    while (true) {
        std::cin.getline(buffer, BUFFER_SIZE);
        int bytesSent = send(clientSocket, buffer, (int)strlen(buffer), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
