#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT "8080"
#define BUFFER_SIZE 1024

std::ofstream logFile;

void log(const std::string& message, const std::string& level) {
    std::time_t now = std::time(nullptr);
    logFile << "["  << "][" << level << "] " << message << std::endl;
}

bool authenticate(SOCKET serverSocket) {
    std::string credentials = "username:password"; // Example credentials
    send(serverSocket, credentials.c_str(), credentials.size(), 0);

    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(serverSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        log("Authentication failed: No data received", "ERROR");
        return false;
    }

    buffer[bytesReceived] = '\0';
    std::string response(buffer);

    if (response == "authenticated") {
        log("Server authenticated successfully", "INFO");
        return true;
    }
    else {
        log("Authentication failed: Server rejected credentials", "ERROR");
        return false;
    }
}

bool handshake(SOCKET serverSocket) {
    const std::string secretMessage = "secret";
    char buffer[BUFFER_SIZE];

    int bytesReceived = recv(serverSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        log("Handshake failed: No data received", "ERROR");
        return false;
    }

    buffer[bytesReceived] = '\0';
    if (secretMessage == buffer) {
        send(serverSocket, secretMessage.c_str(), secretMessage.size(), 0);
        log("Handshake successful", "INFO");
        return true;
    }
    else {
        log("Handshake failed: Secret message mismatch", "ERROR");
        return false;
    }
}

void chat(SOCKET serverSocket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        std::string message;
        std::getline(std::cin, message);
        send(serverSocket, message.c_str(), message.size(), 0);

        int bytesReceived = recv(serverSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Server: " << buffer << std::endl;
        }
        else if (bytesReceived == 0) {
            log("Server disconnected", "INFO");
            closesocket(serverSocket);
            break;
        }
        else {
            log("Receive failed: " + std::to_string(WSAGetLastError()), "ERROR");
            closesocket(serverSocket);
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL;
    struct addrinfo hints;

    logFile.open("client_log.txt", std::ios::out | std::ios::app);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log("WSAStartup failed: " + std::to_string(WSAGetLastError()), "ERROR");
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(SERVER_IP, PORT, &hints, &result) != 0) {
        log("getaddrinfo failed: " + std::to_string(WSAGetLastError()), "ERROR");
        WSACleanup();
        return 1;
    }

    serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (serverSocket == INVALID_SOCKET) {
        log("Socket creation failed: " + std::to_string(WSAGetLastError()), "ERROR");
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (connect(serverSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        log("Connect failed: " + std::to_string(WSAGetLastError()), "ERROR");
        closesocket(serverSocket);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    if (!authenticate(serverSocket)) {
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (!handshake(serverSocket)) {
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    chat(serverSocket);

    closesocket(serverSocket);
    WSACleanup();
    logFile.close();
    return 0;
}
