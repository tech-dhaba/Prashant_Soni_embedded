#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <schannel.h>
#include <security.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Secur32.lib")

#define PORT "8080"
#define BUFFER_SIZE 1024

void InitWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed. Error Code: " << WSAGetLastError() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void CleanupWinsock() {
    WSACleanup();
}

SOCKET CreateServerSocket() {
    struct addrinfo hints, * result;
    SOCKET listenSocket = INVALID_SOCKET;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed. Error Code: " << WSAGetLastError() << std::endl;
        CleanupWinsock();
        exit(EXIT_FAILURE);
    }

    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed. Error Code: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        CleanupWinsock();
        exit(EXIT_FAILURE);
    }

    if (bind(listenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Bind failed. Error Code: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(listenSocket);
        CleanupWinsock();
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed. Error Code: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        CleanupWinsock();
        exit(EXIT_FAILURE);
    }

    return listenSocket;
}

CredHandle hCred;
CtxtHandle hCtxt;
SECURITY_STATUS SecStatus;

void InitializeSchannel() {
    TimeStamp tsExpiry;
    SCHANNEL_CRED SchannelCred;
    ZeroMemory(&SchannelCred, sizeof(SchannelCred));
    SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
    SchannelCred.grbitEnabledProtocols = SP_PROT_TLS1_2_SERVER;
    SchannelCred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION;

    SecStatus = AcquireCredentialsHandle(
        NULL,               // Name of principal
        UNISP_NAME,         // Name of package
        SECPKG_CRED_INBOUND,// Server (inbound) credentials
        NULL,               // Pointer to logon ID
        &SchannelCred,      // Package specific data
        NULL,               // Pointer to GetKey function
        NULL,               // Value to pass to GetKey
        &hCred,             // (out) Cred Handle
        &tsExpiry);         // (out) Lifetime (optional)

    if (SecStatus != SEC_E_OK) {
        std::cerr << "AcquireCredentialsHandle failed with error: " << SecStatus << std::endl;
        exit(EXIT_FAILURE);
    }
}

void PerformSSLHandshake(SOCKET ClientSocket) {
    SecBuffer InBuffers[2], OutBuffers[1];
    SecBufferDesc InBufferDesc, OutBufferDesc;
    DWORD dwSSPIFlags, dwSSPIOutFlags;
    DWORD cbIoBuffer;
    BYTE IoBuffer[BUFFER_SIZE];
    cbIoBuffer = 0;

    // Initialize the buffer descriptor
    InBufferDesc.ulVersion = SECBUFFER_VERSION;
    InBufferDesc.cBuffers = 2;
    InBufferDesc.pBuffers = InBuffers;

    OutBufferDesc.ulVersion = SECBUFFER_VERSION;
    OutBufferDesc.cBuffers = 1;
    OutBufferDesc.pBuffers = OutBuffers;

    // Initialize the buffers
    InBuffers[0].pvBuffer = IoBuffer;
    InBuffers[0].cbBuffer = cbIoBuffer;
    InBuffers[0].BufferType = SECBUFFER_TOKEN;

    InBuffers[1].pvBuffer = NULL;
    InBuffers[1].cbBuffer = 0;
    InBuffers[1].BufferType = SECBUFFER_EMPTY;

    OutBuffers[0].pvBuffer = NULL;
    OutBuffers[0].cbBuffer = 0;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;

    dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT |
        ISC_REQ_REPLAY_DETECT |
        ISC_REQ_CONFIDENTIALITY |
        ISC_RET_EXTENDED_ERROR |
        ISC_REQ_ALLOCATE_MEMORY |
        ISC_REQ_STREAM;

    SecStatus = AcceptSecurityContext(
        &hCred,
        NULL,
        &InBufferDesc,
        dwSSPIFlags,
        SECURITY_NATIVE_DREP,
        &hCtxt,
        &OutBufferDesc,
        &dwSSPIOutFlags,
        NULL);

    if (SecStatus != SEC_I_CONTINUE_NEEDED) {
        std::cerr << "AcceptSecurityContext failed with error: " << SecStatus << std::endl;
        exit(EXIT_FAILURE);
    }

    if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL) {
        send(ClientSocket, (char*)OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
        FreeContextBuffer(OutBuffers[0].pvBuffer);
    }
}

void CleanupSchannel() {
    FreeCredentialsHandle(&hCred);
    DeleteSecurityContext(&hCtxt);
}

int main() {
    InitWinsock();

    SOCKET listenSocket = CreateServerSocket();
    SOCKET clientSocket = INVALID_SOCKET;

    std::cout << "Server listening on port " << PORT << std::endl;

    clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed. Error Code: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        CleanupWinsock();
        exit(EXIT_FAILURE);
    }

    std::cout << "Client connected" << std::endl;

    InitializeSchannel();
    PerformSSLHandshake(clientSocket);

    // Secure communication logic goes here

    closesocket(clientSocket);
    closesocket(listenSocket);
    CleanupSchannel();
    CleanupWinsock();

    return 0;
}
