//example for IOCP i/o completion port 

#include <iostream>
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <thread>
#pragma comment(lib, "Ws2_32.lib")

#define PORT "54000"
#define BUFFER_SIZE 512
#define WORKER_THREADS 4

struct PER_IO_OPERATION_DATA {
    WSAOVERLAPPED overlapped;
    SOCKET socket;
    WSABUF buffer;
    char dataBuffer[BUFFER_SIZE];
    DWORD bytesSent;
    DWORD bytesReceived;
};

struct PER_SOCKET_DATA {
    SOCKET socket;
};

HANDLE completionPort;

void WorkerThread() {
    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    PER_IO_OPERATION_DATA* perIoData;
    BOOL result;

    while (true) {
        result = GetQueuedCompletionStatus(completionPort, &bytesTransferred, &completionKey, (LPOVERLAPPED*)&perIoData, INFINITE);
        if (!result) {
            std::cerr << "GetQueuedCompletionStatus failed: " << GetLastError() << std::endl;
            continue;
        }

        if (bytesTransferred == 0) {
            // Client disconnected
            closesocket(perIoData->socket);
            delete perIoData;
            continue;
        }

        if (perIoData->bytesReceived == 0) {
            // New data received, echo it back
            perIoData->buffer.len = bytesTransferred;
            perIoData->bytesSent = 0;
            perIoData->bytesReceived = bytesTransferred;
        }
        else {
            // Data sent to the client, check if all data is sent
            perIoData->bytesSent += bytesTransferred;
            if (perIoData->bytesSent < perIoData->bytesReceived) {
                // Send remaining data
                perIoData->buffer.buf = perIoData->dataBuffer + perIoData->bytesSent;
                perIoData->buffer.len = perIoData->bytesReceived - perIoData->bytesSent;
            }
            else {
                // All data sent, post a receive to get new data
                perIoData->buffer.buf = perIoData->dataBuffer;
                perIoData->buffer.len = BUFFER_SIZE;
                perIoData->bytesReceived = 0;
                perIoData->bytesSent = 0;
            }
        }

        DWORD flags = 0;
        if (perIoData->bytesReceived == 0) {
            // Post a receive operation
            result = WSARecv(perIoData->socket, &perIoData->buffer, 1, nullptr, &flags, &perIoData->overlapped, nullptr);
        }
        else {
            // Post a send operation
            result = WSASend(perIoData->socket, &perIoData->buffer, 1, nullptr, 0, &perIoData->overlapped, nullptr);
        }

        if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cerr << "WSARecv/WSASend failed: " << WSAGetLastError() << std::endl;
            closesocket(perIoData->socket);
            delete perIoData;
        }
    }
}

int main() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    struct addrinfo* addrInfo = nullptr, hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(nullptr, PORT, &hints, &addrInfo);
    if (result != 0) {
        std::cerr << "getaddrinfo failed: " << result << std::endl;
        WSACleanup();
        return 1;
    }

    SOCKET listenSocket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrInfo);
        WSACleanup();
        return 1;
    }

    result = bind(listenSocket, addrInfo->ai_addr, static_cast<int>(addrInfo->ai_addrlen));
    if (result == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(addrInfo);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(addrInfo);

    result = listen(listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Create IOCP
    completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, WORKER_THREADS);
    if (completionPort == nullptr) {
        std::cerr << "CreateIoCompletionPort failed: " << GetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Start worker threads
    for (int i = 0; i < WORKER_THREADS; ++i) {
        std::thread(WorkerThread).detach();
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        // Associate the socket with the completion port
        CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), completionPort, 0, 0);

        // Allocate memory for per I/O operation data
        PER_IO_OPERATION_DATA* perIoData = new PER_IO_OPERATION_DATA();
        ZeroMemory(&perIoData->overlapped, sizeof(WSAOVERLAPPED));
        perIoData->socket = clientSocket;
        perIoData->buffer.buf = perIoData->dataBuffer;
        perIoData->buffer.len = BUFFER_SIZE;
        perIoData->bytesSent = 0;
        perIoData->bytesReceived = 0;

        // Post an initial receive operation
        DWORD flags = 0;
        result = WSARecv(clientSocket, &perIoData->buffer, 1, nullptr, &flags, &perIoData->overlapped, nullptr);
        if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cerr << "WSARecv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            delete perIoData;
        }
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
