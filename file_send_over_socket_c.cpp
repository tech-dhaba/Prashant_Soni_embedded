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

    // Prepare FileInfo structure
    FileInfo fileInfo;
    strcpy(fileInfo.filename, "test.txt");
    fileInfo.filesize = filesize;

    // Send FileInfo structure
    if (send(ConnectSocket, (char*)&fileInfo, sizeof(FileInfo), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send file info." << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Send file content
    char* buffer = new char[filesize];
    infile.read(buffer, filesize);
    if (send(ConnectSocket, buffer, filesize, 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send file content." << std::endl;
        delete[] buffer;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "File sent successfully." << std::endl;

    // Clean up
    delete[] buffer;
    infile.close();
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}
