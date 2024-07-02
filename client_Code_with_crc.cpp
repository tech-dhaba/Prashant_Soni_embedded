#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")

uint32_t crc32(const uint8_t* data, size_t length);

int main() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddress;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    // Create a socket
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    // Setup server address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    // Connect to server
    if (connect(ConnectSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connect failed." << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Open the file
    std::ifstream infile("test.txt", std::ios::binary);
    if (!infile) {
        std::cerr << "Failed to open file." << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Get file size
    infile.seekg(0, std::ios::end);
    int filesize = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // Read file content
    char* buffer = new char[filesize];
    infile.read(buffer, filesize);
    infile.close();

    bool valid = false;
    while (!valid) {
        // Send file content
        if (send(ConnectSocket, buffer, filesize, 0) == SOCKET_ERROR) {
            std::cerr << "Failed to send file content." << std::endl;
            delete[] buffer;
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        // Receive CRC from server
        uint32_t serverCRC;
        if (recv(ConnectSocket, (char*)&serverCRC, sizeof(serverCRC), 0) <= 0) {
            std::cerr << "Failed to receive CRC from server." << std::endl;
            delete[] buffer;
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        // Calculate CRC
        uint32_t clientCRC = crc32((uint8_t*)buffer, filesize);

        // Validate CRC
        if (serverCRC == clientCRC) {
            valid = true;
            std::cout << "CRC validated successfully." << std::endl;
        } else {
            std::cerr << "CRC validation failed. Resending data." << std::endl;
        }
    }

    // Clean up
    delete[] buffer;
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

uint32_t crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    while (length--) {
        uint8_t byte = *data++;
        crc = crc ^ byte;
        for (int i = 0; i < 8; i++) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}
