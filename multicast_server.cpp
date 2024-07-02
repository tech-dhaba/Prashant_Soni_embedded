#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996) 
#define MULTICAST_PORT 54001
#define MULTICAST_IP "192.168.255.255"
#define BUFFER_SIZE 512

int main() {
    WSADATA wsaData;
    SOCKET sendSocket = INVALID_SOCKET;
    sockaddr_in multicastAddr;
    char sendBuf[BUFFER_SIZE] = "This is a multicast message!";
    int sendBufLen = strlen(sendBuf);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Create a socket
    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sendSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Set up the multicast address
    multicastAddr.sin_family = AF_INET;
    multicastAddr.sin_port = htons(MULTICAST_PORT);
    multicastAddr.sin_addr.s_addr = inet_addr(MULTICAST_IP);

    // Send a multicast message
    if (sendto(sendSocket, sendBuf, sendBufLen, 0, (sockaddr*)&multicastAddr, sizeof(multicastAddr)) == SOCKET_ERROR) {
        std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
        closesocket(sendSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Multicast message sent." << std::endl;

    // Cleanup
    closesocket(sendSocket);
    WSACleanup();

    return 0;
}
