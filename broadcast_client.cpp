#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996) 
#define BROADCAST_PORT 54000
#define BUFFER_SIZE 512

int main() {
    WSADATA wsaData;
    SOCKET recvSocket = INVALID_SOCKET;
    sockaddr_in recvAddr;
    char recvBuf[BUFFER_SIZE];
    int recvBufLen = BUFFER_SIZE;
    sockaddr_in senderAddr;
    int senderAddrSize = sizeof(senderAddr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Create a socket
    recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (recvSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Set up the receive address
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_port = htons(BROADCAST_PORT);
    recvAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(recvSocket, (sockaddr*)&recvAddr, sizeof(recvAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(recvSocket);
        WSACleanup();
        return 1;
    }

    // Receive a broadcast message
    int bytesReceived = recvfrom(recvSocket, recvBuf, recvBufLen, 0, (sockaddr*)&senderAddr, &senderAddrSize);
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "recvfrom failed: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Broadcast message received: " << std::string(recvBuf, bytesReceived) << std::endl;
    }

    // Cleanup
    closesocket(recvSocket);
    WSACleanup();

    return 0;
}
