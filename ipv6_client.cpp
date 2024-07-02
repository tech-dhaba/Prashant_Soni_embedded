#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT "8080"
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in6 server_addr;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed. Error Code : " << WSAGetLastError() << std::endl;
        return 1;
    }

    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed. Error Code : " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(std::stoi(PORT));
    //the IPv6 loopback address ::1.
   // IPv4 loopback address 127.0.0.1,
    if (inet_pton(AF_INET6, "::1", &server_addr.sin6_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int activity = select(0, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            std::cerr << "select error" << std::endl;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::cin.getline(buffer, BUFFER_SIZE);
            if (send(sock, buffer, strlen(buffer), 0) == SOCKET_ERROR) {
                std::cerr << "send failed. Error Code : " << WSAGetLastError() << std::endl;
            }
        }

        if (FD_ISSET(sock, &readfds)) {
            int valread = recv(sock, buffer, BUFFER_SIZE, 0);
            if (valread == SOCKET_ERROR) {
                std::cerr << "recv failed. Error Code : " << WSAGetLastError() << std::endl;
                continue;
            }
            else if (valread == 0) {
                std::cout << "Server disconnected" << std::endl;
                break;
            }

            buffer[valread] = '\0';
            std::cout << "Server: " << buffer << std::endl;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
