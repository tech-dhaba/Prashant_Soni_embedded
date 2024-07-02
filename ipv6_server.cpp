#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT "8080"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main() {
    WSADATA wsaData;
    SOCKET listen_socket, client_sockets[MAX_CLIENTS];
    struct sockaddr_in6 server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed. Error Code : " << WSAGetLastError() << std::endl;
        return 1;
    }

    if ((listen_socket = socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed. Error Code : " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(std::stoi(PORT));

    if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed. Error Code : " << WSAGetLastError() << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(listen_socket, &readfds);

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (client_sockets[i] != 0) {
                FD_SET(client_sockets[i], &readfds);
            }
        }

        int activity = select(0, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            std::cerr << "select error" << std::endl;
        }

        if (FD_ISSET(listen_socket, &readfds)) {
            if ((client_sockets[0] = accept(listen_socket, (struct sockaddr*)&client_addr, &addr_len)) == INVALID_SOCKET) {
                std::cerr << "Accept failed. Error Code : " << WSAGetLastError() << std::endl;
                continue;
            }

            std::cout << "New connection, socket fd is " << client_sockets[0]
                << ", ip is : " << inet_ntop(AF_INET6, &client_addr.sin6_addr, buffer, sizeof(buffer))
                << ", port : " << ntohs(client_addr.sin6_port) << std::endl;

            // Add new socket to array of sockets
            for (int i = 1; i < MAX_CLIENTS; ++i) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_sockets[0];
                    break;
                }
            }
        }

        for (int i = 1; i < MAX_CLIENTS; ++i) {
            if (FD_ISSET(client_sockets[i], &readfds)) {
                int valread = recv(client_sockets[i], buffer, BUFFER_SIZE, 0);
                if (valread == SOCKET_ERROR) {
                    std::cerr << "recv failed. Error Code : " << WSAGetLastError() << std::endl;
                    closesocket(client_sockets[i]);
                    client_sockets[i] = 0;
                    continue;
                }
                else if (valread == 0) {
                    std::cout << "Client disconnected. Socket fd is " << client_sockets[i] << std::endl;
                    closesocket(client_sockets[i]);
                    client_sockets[i] = 0;
                    continue;
                }

                buffer[valread] = '\0';
                std::cout << "Client " << i << ": " << buffer << std::endl;

                if (send(client_sockets[i], buffer, valread, 0) == SOCKET_ERROR) {
                    std::cerr << "send failed. Error Code : " << WSAGetLastError() << std::endl;
                }
            }
        }
    }

    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
