#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <bcrypt.h>
#include <vector>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Bcrypt.lib")

#define SERVER_IP "127.0.0.1"
#define PORT "8080"
#define BUFFER_SIZE 1024

std::ofstream logFile;

void log(const std::string& message) {
    std::time_t now = std::time(nullptr);
    logFile << "] " << message << std::endl;
}

void HandleError(const std::string& function, NTSTATUS status) {
    std::cerr << function << " failed with status: " << status << std::endl;
    log(function + " failed with status: " + std::to_string(status));
    exit(1);
}

void EncryptData(BCRYPT_KEY_HANDLE hKey, const std::vector<BYTE>& plaintext, std::vector<BYTE>& ciphertext) {
    DWORD cbCipherText = 0, cbData = 0;

    NTSTATUS status = BCryptEncrypt(hKey, (PUCHAR)plaintext.data(), (ULONG)plaintext.size(), NULL, NULL, 0, NULL, 0, &cbCipherText, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status)) HandleError("BCryptEncrypt (query size)", status);

    ciphertext.resize(cbCipherText);

    status = BCryptEncrypt(hKey, (PUCHAR)plaintext.data(), (ULONG)plaintext.size(), NULL, NULL, 0, ciphertext.data(), (ULONG)ciphertext.size(), &cbData, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status)) HandleError("BCryptEncrypt", status);
}

void DecryptData(BCRYPT_KEY_HANDLE hKey, const std::vector<BYTE>& ciphertext, std::vector<BYTE>& plaintext) {
    DWORD cbPlainText = 0, cbData = 0;

    NTSTATUS status = BCryptDecrypt(hKey, (PUCHAR)ciphertext.data(), (ULONG)ciphertext.size(), NULL, NULL, 0, NULL, 0, &cbPlainText, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status)) HandleError("BCryptDecrypt (query size)", status);

    plaintext.resize(cbPlainText);

    status = BCryptDecrypt(hKey, (PUCHAR)ciphertext.data(), (ULONG)ciphertext.size(), NULL, NULL, 0, plaintext.data(), (ULONG)plaintext.size(), &cbData, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status)) HandleError("BCryptDecrypt", status);
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL;
    struct addrinfo hints;
    BCRYPT_ALG_HANDLE hAesAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    std::vector<BYTE> key = { /* 16-byte key */ 0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE, 0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81 };

    logFile.open("client_log.txt", std::ios::out | std::ios::app);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        log("WSAStartup failed");
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(SERVER_IP, PORT, &hints, &result) != 0) {
        log("getaddrinfo failed");
        WSACleanup();
        return 1;
    }

    clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (clientSocket == INVALID_SOCKET) {
        log("Socket creation failed");
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (connect(clientSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        log("Connect failed");
        closesocket(clientSocket);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAesAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status)) HandleError("BCryptOpenAlgorithmProvider", status);

    status = BCryptGenerateSymmetricKey(hAesAlg, &hKey, NULL, 0, key.data(), (ULONG)key.size(), 0);
    if (!BCRYPT_SUCCESS(status)) HandleError("BCryptGenerateSymmetricKey", status);

    while (true) {
        std::string message;
        std::getline(std::cin, message);
        std::vector<BYTE> plainText(message.begin(), message.end());
        std::vector<BYTE> cipherText;
        EncryptData(hKey, plainText, cipherText);

        send(clientSocket, (char*)cipherText.data(), cipherText.size(), 0);

        char recvBuffer[BUFFER_SIZE];
        int bytesReceived = recv(clientSocket, recvBuffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            std::vector<BYTE> encryptedData(recvBuffer, recvBuffer + bytesReceived);
            std::vector<BYTE> decryptedData;
            DecryptData(hKey, encryptedData, decryptedData);

            std::string reply(decryptedData.begin(), decryptedData.end());
            log("Server: " + reply);
        }
        else if (bytesReceived == 0) {
            log("Server disconnected");
            closesocket(clientSocket);
            break;
        }
        else {
            log("Receive failed");
            closesocket(clientSocket);
            break;
        }
    }

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAesAlg, 0);
    closesocket(clientSocket);
    WSACleanup();
    logFile.close();
    return 0;
}
