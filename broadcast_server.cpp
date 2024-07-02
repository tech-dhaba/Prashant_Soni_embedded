#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996) 
#define BROADCAST_PORT 54000
#define BROADCAST_IP "255.255.255.255"
#define BUFFER_SIZE 512

int main() {
    WSADATA wsaData;
    SOCKET sendSocket = INVALID_SOCKET;
    sockaddr_in broadcastAddr;
    int broadcast = 1;
    char sendBuf[BUFFER_SIZE] = "This is a broadcast message!";
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

    // Set socket options to enable broadcast
    if (setsockopt(sendSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) {
        std::cerr << "setsockopt failed: " << WSAGetLastError() << std::endl;
        closesocket(sendSocket);
        WSACleanup();
        return 1;
    }

    // Set up the broadcast address
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(BROADCAST_PORT);
    broadcastAddr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Send a broadcast message
    if (sendto(sendSocket, sendBuf, sendBufLen, 0, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == SOCKET_ERROR) {
        std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
        closesocket(sendSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Broadcast message sent." << std::endl;

    // Cleanup
    closesocket(sendSocket);
    WSACleanup();

    return 0;
}
