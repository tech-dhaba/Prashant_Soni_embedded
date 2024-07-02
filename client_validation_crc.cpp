#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <stdint.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT "8080"
#define SERVER_IP "::1"
#define BUFFER_SIZE 1024
#define ENCRYPTION_KEY 0xAA

struct ValidationStructure {
    int id;
    char message[50];
};

uint32_t crc32(const char* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    while (length--) {
        crc ^= *data++;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

void xor_encrypt_decrypt(char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        data[i] ^= ENCRYPTION_KEY;
    }
}

int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in6 server_addr;
    char buffer[BUFFER_SIZE];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed. Error Code : " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Create socket
    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed. Error Code : " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Prepare server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(std::stoi(PORT));

    // Convert IPv6 address from text to binary form
    if (inet_pton(AF_INET6, SERVER_IP, &server_addr.sin6_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    // Send validation structure to server
    ValidationStructure client_val_struct = { 123, "Hello from client" };
    if (send(sock, (char*)&client_val_struct, sizeof(client_val_struct), 0) == SOCKET_ERROR) {
        std::cerr << "send failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Receive validation structure from server
    ValidationStructure val_struct;
    int valread = recv(sock, (char*)&val_struct, sizeof(val_struct), 0);
    if (valread <= 0) {
        std::cerr << "recv failed or connection closed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Received validation structure from server. ID: " << val_struct.id << ", Message: " << val_struct.message << std::endl;

    // Perform handshake
    valread = recv(sock, buffer, BUFFER_SIZE, 0);
    xor_encrypt_decrypt(buffer, valread);
    if (valread <= 0 || strncmp(buffer, "HANDSHAKE", valread) != 0) {
        std::cerr << "Handshake failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    const char* handshake_msg = "HANDSHAKE";
    xor_encrypt_decrypt((char*)handshake_msg, strlen(handshake_msg));
    if (send(sock, handshake_msg, strlen(handshake_msg), 0) == SOCKET_ERROR) {
        std::cerr << "send failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Handshake successful." << std::endl;

    // Send data to server
    std::string message = "Data from client";
    xor_encrypt_decrypt((char*)message.c_str(), message.length());
    if (send(sock, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
        std::cerr << "send failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Receive CRC acknowledgment from server
    uint32_t crc;
    valread = recv(sock, (char*)&crc, sizeof(crc), 0);
    if (valread <= 0) {
        std::cerr << "recv failed or connection closed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Received CRC acknowledgment: " << crc << std::endl;

    // Validate CRC
    uint32_t calculated_crc = crc32(message.c_str(), message.length());
    if (crc == calculated_crc) {
        std::cout << "CRC validation successful." << std::endl;
    }
    else {
        std::cerr << "CRC validation failed." << std::endl;
    }

    // Close socket and cleanup
    closesocket(sock);
    WSACleanup();

    return 0;
}
