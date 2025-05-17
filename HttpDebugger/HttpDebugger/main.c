#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

#define IP "127.0.0.1"
#define PORT 8080
#define BUFFER_SIZE 8192

void LogHTTPRequest(const char* buffer, int bytes) {
    printf("----- HTTP REQUEST -----\n");
    fwrite(buffer, 1, bytes, stdout);
    printf("\n------------------------\n");
}

void relay_data(SOCKET clientSocket, SOCKET serverSocket) {
    char buffer[BUFFER_SIZE];
    int bytes;
    fd_set fdset;
    while (1) {
        FD_ZERO(&fdset);
        FD_SET(clientSocket, &fdset);
        FD_SET(serverSocket, &fdset);

        if (select(0, &fdset, NULL, NULL, NULL) == SOCKET_ERROR) {
            break;
        }

        if (FD_ISSET(clientSocket, &fdset)) {
            bytes = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break;
            send(serverSocket, buffer, bytes, 0);
        }

        if (FD_ISSET(serverSocket, &fdset)) {
            bytes = recv(serverSocket, buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) break;
            send(clientSocket, buffer, bytes, 0);
        }
    }
}

void handle_connect(SOCKET clientSocket, char* host, int port) {
    SOCKET serverSocket;
    struct sockaddr_in serverAddr;
    struct hostent* he;

    he = gethostbyname(host);
    if (!he) {
        printf("[!] DNS resolution failed for host: %s\n", host);
        return;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("[!] Failed to create server socket\n");
        return;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    memcpy(&serverAddr.sin_addr, he->h_addr, he->h_length);

    printf("[*] Connecting to %s:%d...\n", host, port);
    if (connect(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("[!] Failed to connect to %s:%d\n", host, port);
        closesocket(serverSocket);
        return;
    }

    printf("[*] Connected to %s:%d\n", host, port);

    const char* response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(clientSocket, response, strlen(response), 0);

    relay_data(clientSocket, serverSocket);

    closesocket(serverSocket);
}

void HttpMonitor() {
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        printf("[!] WSAStartup failed\n");
        return;
    }

    SOCKET listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) {
        printf("[!] Failed to create socket\n");
        WSACleanup();
        return;
    }

    struct sockaddr_in proxyAddr;
    memset(&proxyAddr, 0, sizeof(proxyAddr));
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = inet_addr(IP);
    proxyAddr.sin_port = htons(PORT);

    if (bind(listener, (SOCKADDR*)&proxyAddr, sizeof(proxyAddr)) == SOCKET_ERROR) {
        printf("[!] Bind failed with error: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return;
    }

    if (listen(listener, SOMAXCONN) == SOCKET_ERROR) {
        printf("[!] Listen failed with error: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return;
    }

    printf("[*] Proxy listening on %s:%d...\n", IP, PORT);

    while (1) {
        SOCKET clientSocket = accept(listener, NULL, NULL);

        if (clientSocket == INVALID_SOCKET) {
            printf("[!] Accept failed: %d\n", WSAGetLastError());
            continue;
        }

        printf("[+] Accepted connection\n");

        char buffer[BUFFER_SIZE];
        int bytes = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            printf("[!] Failed to receive data from client\n");
            closesocket(clientSocket);
            continue;
        }

        buffer[bytes] = '\0';

        if (strncmp(buffer, "CONNECT", 7) == 0) {
            char* host_start = buffer + 8;
            char* host_end = strchr(host_start, ' ');
            if (!host_end) {
                printf("[!] Malformed CONNECT request\n");
                closesocket(clientSocket);
                continue;
            }
            *host_end = '\0';

            char* colon = strchr(host_start, ':');
            if (!colon) {
                printf("[!] Malformed CONNECT host:port\n");
                closesocket(clientSocket);
                continue;
            }
            *colon = '\0';
            char* host = host_start;
            int port = atoi(colon + 1);

            printf("[*] CONNECT to %s:%d\n", host, port);
            handle_connect(clientSocket, host, port);
            closesocket(clientSocket);
            printf("[*] CONNECT tunnel closed\n");
            continue;
        }

        LogHTTPRequest(buffer, bytes);

        char* hostHeader = strstr(buffer, "Host: ");
        if (!hostHeader) {
            printf("[!] Host header not found\n");
            closesocket(clientSocket);
            continue;
        }

        hostHeader += 6;
        char* end = strstr(hostHeader, "\r\n");
        if (!end) {
            printf("[!] Malformed Host header\n");
            closesocket(clientSocket);
            continue;
        }

        char host[256] = { 0 };
        int len = end - hostHeader;
        if (len >= sizeof(host)) len = sizeof(host) - 1;
        strncpy(host, hostHeader, len);
        host[len] = '\0';

        printf("[*] Extracted host: %s\n", host);

        struct hostent* he = gethostbyname(host);
        if (!he) {
            printf("[!] DNS resolution failed for host: %s\n", host);
            closesocket(clientSocket);
            continue;
        }

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            printf("[!] Failed to create server socket\n");
            closesocket(clientSocket);
            continue;
        }

        struct sockaddr_in serverAddr;

        memset(&serverAddr, 0, sizeof(serverAddr));
        
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(80);
        memcpy(&serverAddr.sin_addr, he->h_addr, he->h_length);

        printf("[*] Connecting to %s...\n", host);

        if (connect(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            printf("[!] Failed to connect to %s\n", host);
            closesocket(serverSocket);
            closesocket(clientSocket);
            continue;
        }

        printf("[*] Connected to %s\n", host);

        send(serverSocket, buffer, bytes, 0);

        while ((bytes = recv(serverSocket, buffer, BUFFER_SIZE, 0)) > 0) {
            send(clientSocket, buffer, bytes, 0);
        }

        closesocket(serverSocket);
        closesocket(clientSocket);
        printf("[*] Connection closed\n");
    }

    closesocket(listener);
    
    WSACleanup();
}

int main(void) {
    HttpMonitor();
    return 0;
}
