#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

struct FileInfo {
    char filename[256];
    int filesize;
};

int main() {
    WSADATA wsaData;
    SOCKET ListenSocket = INVALID_SOCKET, ClientSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddress;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    // Create a socket
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    // Setup server address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(ListenSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Listen on the socket
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Waiting for client to connect..." << std::endl;

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed." << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Receive FileInfo structure
    FileInfo fileInfo;
    if (recv(ClientSocket, (char*)&fileInfo, sizeof(FileInfo), 0) <= 0) {
        std::cerr << "Failed to receive file info." << std::endl;
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Receive file content
    char* buffer = new char[fileInfo.filesize];
    int bytesReceived = 0;
    while (bytesReceived < fileInfo.filesize) {
        int result = recv(ClientSocket, buffer + bytesReceived, fileInfo.filesize - bytesReceived, 0);
        if (result <= 0) {
            std::cerr << "Failed to receive file content." << std::endl;
            delete[] buffer;
            closesocket(ClientSocket);
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        bytesReceived += result;
    }

    // Write file content to a new file
    std::ofstream outfile(fileInfo.filename, std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to open output file." << std::endl;
        delete[] buffer;
        closesocket(ClientSocket);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    outfile.write(buffer, fileInfo.filesize);
    outfile.close();

    std::cout << "File received and saved successfully." << std::endl;

    // Clean up
    delete[] buffer;
    closesocket(ClientSocket);
    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}
