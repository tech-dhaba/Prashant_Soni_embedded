// RawSocketClient.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996) 
#define ICMP_ECHO 8
#define ICMP_ECHO_REPLY 0
#define DEFAULT_BUFLEN 512

struct icmp_header {
    unsigned char type;
    unsigned char code;
    unsigned short checksum;
    unsigned short id;
    unsigned short sequence;
};

unsigned short checksum(void* buf, int len) {
    unsigned short* buffer = (unsigned short*)buf;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buffer++;
    if (len == 1)
        sum += *(unsigned char*)buffer;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main() {
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    SOCKET rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (rawSocket == INVALID_SOCKET) {
        printf("socket failed: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    char sendbuf[DEFAULT_BUFLEN];
    memset(sendbuf, 0, DEFAULT_BUFLEN);

    struct icmp_header* icmp = (struct icmp_header*)sendbuf;
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = (unsigned short)GetCurrentProcessId();
    icmp->sequence = 0;

    // Calculate checksum
    icmp->checksum = checksum(sendbuf, sizeof(struct icmp_header));

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");  // Target IP address

    iResult = sendto(rawSocket, sendbuf, sizeof(struct icmp_header), 0, (struct sockaddr*)&dest, sizeof(dest));
    if (iResult == SOCKET_ERROR) {
        printf("sendto failed: %d\n", WSAGetLastError());
        closesocket(rawSocket);
        WSACleanup();
        return 1;
    }

    printf("Packet sent\n");

    closesocket(rawSocket);
    WSACleanup();

    return 0;
}
