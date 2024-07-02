#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_ADDRESS "127.0.0.1"
#define PORT "54000"
#define BUFFER_SIZE 512

int main() {
    WSADATA wsaData;
    int result;

    // Initialize Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    struct addrinfo* addrInfo = nullptr, hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    result = getaddrinfo(SERVER_ADDRESS, PORT, &hints, &addrInfo);
    if (result != 0) {
        std::cerr << "getaddrinfo failed: " << result << std::endl;
        WSACleanup();
        return 1;
    }

    // Create a socket for connecting to the server
    SOCKET ConnectSocket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrInfo);
        WSACleanup();
        return 1;
    }

    // Connect to the server
    result = connect(ConnectSocket, addrInfo->ai_addr, static_cast<int>(addrInfo->ai_addrlen));
    if (result == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        freeaddrinfo(addrInfo);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(addrInfo);

    std::string message = "Hello, server!";
    char buffer[BUFFER_SIZE];

    // Send an initial buffer
    result = send(ConnectSocket, message.c_str(), static_cast<int>(message.length()), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Bytes sent: " << result << std::endl;

    // Receive data from the server
    result = recv(ConnectSocket, buffer, BUFFER_SIZE, 0);
    if (result > 0) {
        std::cout << "Bytes received: " << result << std::endl;
        std::cout << "Echo from server: " << std::string(buffer, result) << std::endl;
    }
    else if (result == 0) {
        std::cout << "Connection closed" << std::endl;
    }
    else {
        std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
    }

    // Cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}
