#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <stdint.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT "8080"
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
    SOCKET listen_socket, client_socket;
    struct sockaddr_in6 server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed. Error Code : " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Create socket for listening
    if ((listen_socket = socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed. Error Code : " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Prepare server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(std::stoi(PORT));

    // Bind socket to address
    if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Accept a client connection
    if ((client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &addr_len)) == INVALID_SOCKET) {
        std::cerr << "Accept failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    // Receive validation structure from client
    ValidationStructure val_struct;
    int valread = recv(client_socket, (char*)&val_struct, sizeof(val_struct), 0);
    if (valread <= 0) {
        std::cerr << "recv failed or connection closed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Received validation structure from client. ID: " << val_struct.id << ", Message: " << val_struct.message << std::endl;
    //validate
    // Send validation structure to client
    ValidationStructure server_val_struct = { 456, "Hello from server" };
    if (send(client_socket, (char*)&server_val_struct, sizeof(server_val_struct), 0) == SOCKET_ERROR) {
        std::cerr << "send failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    // Perform handshake
    const char* handshake_msg = "HANDSHAKE";
    xor_encrypt_decrypt((char*)handshake_msg, strlen(handshake_msg));
    if (send(client_socket, handshake_msg, strlen(handshake_msg), 0) == SOCKET_ERROR) {
        std::cerr << "send failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
    xor_encrypt_decrypt(buffer, valread);
    if (valread <= 0 || strncmp(buffer, "HANDSHAKE", valread) != 0) {
        std::cerr << "Handshake failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Handshake successful." << std::endl;

    // Receive data from client
    valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
    xor_encrypt_decrypt(buffer, valread);
    if (valread <= 0) {
        std::cerr << "recv failed or connection closed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(client_socket);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    buffer[valread] = '\0';
    std::cout << "Received from client: " << buffer << std::endl;

    // Calculate and send CRC as acknowledgment
    uint32_t crc = crc32(buffer, valread);
    if (send(client_socket, (char*)&crc, sizeof(crc), 0) == SOCKET_ERROR) {
        std::cerr << "send failed. Error Code : " << WSAGetLastError() << std::endl;
    }

    // Close sockets and cleanup
    closesocket(client_socket);
    closesocket(listen_socket);
    WSACleanup();

    return 0;
}
