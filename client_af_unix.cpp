// NamedPipeClient.cpp
#include <windows.h>
#include <iostream>
#include <thread>

#define PIPE_NAME "\\\\.\\pipe\\MyPipe"

void receiveMessages(HANDLE hPipe) {
    char buffer[512];
    DWORD bytesRead;

    while (true) {
        BOOL success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        if (!success || bytesRead == 0) {
            std::cout << "Read error or server disconnected.\n";
            break;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Received: " << buffer << "\n";
    }
}

int main() {
    HANDLE hPipe;
    DWORD bytesWritten;
    char buffer[512];

    hPipe = CreateFile(
        PIPE_NAME,           // pipe name
        GENERIC_READ |       // read and write access
        GENERIC_WRITE,
        0,                   // no sharing
        NULL,                // default security attributes
        OPEN_EXISTING,       // opens existing pipe
        0,                   // default attributes
        NULL);               // no template file

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cout << "CreateFile failed, GLE=" << GetLastError() << "\n";
        return 1;
    }

    std::thread recvThread(receiveMessages, hPipe);

    while (true) {
        std::cin.getline(buffer, sizeof(buffer));
        BOOL success = WriteFile(hPipe, buffer, strlen(buffer), &bytesWritten, NULL);
        if (!success) {
            std::cout << "WriteFile failed, GLE=" << GetLastError() << "\n";
            break;
        }
    }

    recvThread.join();
    CloseHandle(hPipe);

    return 0;
}
