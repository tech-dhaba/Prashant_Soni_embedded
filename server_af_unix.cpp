// NamedPipeServer.cpp
#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>

#define PIPE_NAME "\\\\.\\pipe\\MyPipe"

std::vector<HANDLE> clients;

void handleClient(HANDLE hPipe) {
    char buffer[512];
    DWORD bytesRead, bytesWritten;

    while (true) {
        BOOL success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        if (!success || bytesRead == 0) {
            std::cout << "Client disconnected or read error.\n";
            break;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Received: " << buffer << "\n";

        // Echo the message back to the client
        success = WriteFile(hPipe, buffer, bytesRead, &bytesWritten, NULL);
        if (!success || bytesRead != bytesWritten) {
            std::cout << "Write error.\n";
            break;
        }
    }

    CloseHandle(hPipe);
    clients.erase(std::remove(clients.begin(), clients.end(), hPipe), clients.end());
}

int main() {
    HANDLE hPipe;
    std::vector<std::thread> clientThreads;

    while (true) {
        hPipe = CreateNamedPipe(
            PIPE_NAME,               // pipe name
            PIPE_ACCESS_DUPLEX,      // read/write access
            PIPE_TYPE_MESSAGE |      // message-type pipe
            PIPE_READMODE_MESSAGE |  // message-read mode
            PIPE_WAIT,               // blocking mode
            PIPE_UNLIMITED_INSTANCES,// max. instances
            512,                     // output buffer size
            512,                     // input buffer size
            0,                       // client time-out
            NULL);                   // default security attribute

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::cout << "CreateNamedPipe failed, GLE=" << GetLastError() << "\n";
            return 1;
        }

        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            std::cout << "Client connected.\n";
            clients.push_back(hPipe);
            clientThreads.push_back(std::thread(handleClient, hPipe));
        } else {
            CloseHandle(hPipe);
        }
    }

    for (auto& th : clientThreads) {
        th.join();
    }

    return 0;
}
