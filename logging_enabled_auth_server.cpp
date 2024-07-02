#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")

#define PORT "8080"
#define BUFFER_SIZE 1024

std::ofstream logFile;

void log(const std::string& message, const std::string& level) {
    std::time_t now = std::time(nullptr);
    logFile << "["  << "][" << level << "] " << message << std::endl;
}

bool authenticate(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        log("Authentication failed: No data received", "ERROR");
        return false;
    }

    buffer[bytesReceived] = '\0';
    std::string receivedCredentials(buffer);

    std::string expectedCredentials = "username:password"; // Example credentials
    if (receivedCredentials == expectedCredentials) {
        log("Client authenticated successfully", "INFO");
        return true;
    }
    else {
        log("Authentication failed: Invalid credentials", "ERROR");
        return false;
    }
}

bool handshake(SOCKET clientSocket) {
    const std::string secretMessage = "secret";
    char buffer[BUFFER_SIZE];

    send(clientSocket, secretMessage.c_str(), secretMessage.size(), 0);
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

    if (bytesReceived <= 0) {
        log("Handshake failed: No data received", "ERROR");
        return false;
    }

    buffer[bytesReceived] = '\0';
    if (secretMessage == buffer) {
        log("Handshake successful", "INFO");
        return true;
    }
    else {
        log("Handshake failed: Secret message mismatch", "ERROR");
        return false;
    }
}

void chat(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Client: " << buffer << std::endl;
            std::string reply;
            std::getline(std::cin, reply);
            send(clientSocket, reply.c_str(), reply.size(), 0);
        }
        else if (bytesReceived == 0) {
            log("Client disconnected", "INFO");
            closesocket(clientSocket);
            break;
        }
        else {
            log("Receive failed: " + std::to_string(WSAGetLastError()), "ERROR");
            closesocket(clientSocket);
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL;
    struct addrinfo hints;

    logFile.open("server_log.txt", std::ios::out | std::ios::app);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log("WSAStartup failed: " + std::to_string(WSAGetLastError()), "ERROR");
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &result) != 0) {
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

    if (bind(serverSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        log("Bind failed: " + std::to_string(WSAGetLastError()), "ERROR");
        freeaddrinfo(result);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        log("Listen failed: " + std::to_string(WSAGetLastError()), "ERROR");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    log("Server listening on port " + std::string(PORT), "INFO");

    clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        log("Accept failed: " + std::to_string(WSAGetLastError()), "ERROR");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (!authenticate(clientSocket)) {
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (!handshake(clientSocket)) {
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    chat(clientSocket);

    closesocket(serverSocket);
    WSACleanup();
    logFile.close();
    return 0;
}
