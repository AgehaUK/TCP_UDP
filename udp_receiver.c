#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define IDC_PORT_EDIT 101
#define IDC_IP_EDIT 102
#define IDC_OPEN_BUTTON 103
#define IDC_MESSAGE_EDIT 104

HWND hPortEdit, hIpEdit, hOpenButton, hMessageEdit;
SOCKET udpSock = INVALID_SOCKET;
BOOL isListening = FALSE;
HANDLE hListenThread = NULL;

typedef struct {
    int port;
    char ipAddress[16];
} Config;

BOOL LoadConfig(Config *config) {
    FILE *fp = fopen("udp_receiver_config.txt", "r");
    if (fp == NULL) {
        return FALSE;
    }
    
    if (fscanf(fp, "%d %15s", &config->port, config->ipAddress) != 2) {
        fclose(fp);
        return FALSE;
    }
    
    fclose(fp);
    
    if (config->port < 1024 || config->port > 65535) {
        return FALSE;
    }
    
    return TRUE;
}

void SaveConfig(int port, const char* ipAddress) {
    FILE *fp = fopen("udp_receiver_config.txt", "w");
    if (fp != NULL) {
        fprintf(fp, "%d %s", port, ipAddress);
        fclose(fp);
    }
}

void CreateDefaultConfigAndExit() {
    SaveConfig(12345, "0.0.0.0");
    MessageBoxA(NULL, 
                "Config file not found.\nCreated udp_receiver_config.txt with default settings.\n(Port: 12345, IP: 0.0.0.0)\n\nThe program will now exit.",
                "First Run", 
                MB_OK | MB_ICONINFORMATION);
    exit(0);
}

void AppendMessage(const char* message) {
    int len = GetWindowTextLengthA(hMessageEdit);
    SendMessageA(hMessageEdit, EM_SETSEL, len, len);
    SendMessageA(hMessageEdit, EM_REPLACESEL, FALSE, (LPARAM)message);
}

DWORD WINAPI ListenThread(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;
    char buffer[1024];
    struct sockaddr_in senderAddr;
    int senderLen = sizeof(senderAddr);
    
    while (isListening) {
        int recvLen = recvfrom(udpSock, buffer, sizeof(buffer) - 1, 0, 
                              (struct sockaddr*)&senderAddr, &senderLen);
        
        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            
            char senderIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(senderAddr.sin_addr), senderIp, INET_ADDRSTRLEN);
            
            char displayMsg[2048];
            sprintf(displayMsg, "[From %s:%d] %s\r\n", 
                    senderIp, ntohs(senderAddr.sin_port), buffer);
            AppendMessage(displayMsg);
        } else if (recvLen == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error != WSAEINTR && isListening) {
                AppendMessage("[Error] Receive error occurred\r\n");
            }
            break;
        }
    }
    
    return 0;
}

void StartListening(HWND hwnd) {
    char port[10];
    char ipAddress[16];
    GetWindowTextA(hPortEdit, port, sizeof(port));
    GetWindowTextA(hIpEdit, ipAddress, sizeof(ipAddress));
    
    int listenPort = atoi(port);
    if (listenPort < 1024 || listenPort > 65535) {
        MessageBoxA(hwnd, "Port must be between 1024-65535", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    SaveConfig(listenPort, ipAddress);
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        MessageBoxA(hwnd, "Failed to initialize Winsock", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock == INVALID_SOCKET) {
        MessageBoxA(hwnd, "Failed to create socket", "Error", MB_OK | MB_ICONERROR);
        WSACleanup();
        return;
    }
    
    struct sockaddr_in udpAddr;
    memset(&udpAddr, 0, sizeof(udpAddr));
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port = htons(listenPort);
    
    if (strcmp(ipAddress, "0.0.0.0") == 0) {
        udpAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, ipAddress, &udpAddr.sin_addr) != 1) {
            MessageBoxA(hwnd, "Invalid IP address", "Error", MB_OK | MB_ICONERROR);
            closesocket(udpSock);
            udpSock = INVALID_SOCKET;
            WSACleanup();
            return;
        }
    }
    
    if (bind(udpSock, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) == SOCKET_ERROR) {
        MessageBoxA(hwnd, "Failed to bind port", "Error", MB_OK | MB_ICONERROR);
        closesocket(udpSock);
        udpSock = INVALID_SOCKET;
        WSACleanup();
        return;
    }
    
    isListening = TRUE;
    EnableWindow(hPortEdit, FALSE);
    EnableWindow(hIpEdit, FALSE);
    SetWindowTextA(hOpenButton, "Listening...");
    EnableWindow(hOpenButton, FALSE);
    
    char msg[256];
    sprintf(msg, "[System] Started listening on %s:%d\r\n", ipAddress, listenPort);
    AppendMessage(msg);
    
    hListenThread = CreateThread(NULL, 0, ListenThread, hwnd, 0, NULL);
}

void StopListening() {
    if (isListening) {
        isListening = FALSE;
        
        if (udpSock != INVALID_SOCKET) {
            closesocket(udpSock);
            udpSock = INVALID_SOCKET;
        }
        
        if (hListenThread != NULL) {
            WaitForSingleObject(hListenThread, 1000);
            CloseHandle(hListenThread);
            hListenThread = NULL;
        }
        
        WSACleanup();
        
        EnableWindow(hPortEdit, TRUE);
        EnableWindow(hIpEdit, TRUE);
        SetWindowTextA(hOpenButton, "Open");
        EnableWindow(hOpenButton, TRUE);
        
        AppendMessage("[System] Stopped listening\r\n");
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            CreateWindowA("STATIC", "IP Address:", WS_VISIBLE | WS_CHILD,
                        20, 20, 100, 20, hwnd, NULL, NULL, NULL);
            
            hIpEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                                    130, 18, 150, 25, hwnd, (HMENU)IDC_IP_EDIT, NULL, NULL);
            
            CreateWindowA("STATIC", "Port:", WS_VISIBLE | WS_CHILD,
                        20, 55, 100, 20, hwnd, NULL, NULL, NULL);
            
            hPortEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                                    130, 53, 100, 25, hwnd, (HMENU)IDC_PORT_EDIT, NULL, NULL);
            
            Config config;
            if (LoadConfig(&config)) {
                char portStr[10];
                sprintf(portStr, "%d", config.port);
                SetWindowTextA(hPortEdit, portStr);
                SetWindowTextA(hIpEdit, config.ipAddress);
            } else {
                SetWindowTextA(hPortEdit, "12345");
                SetWindowTextA(hIpEdit, "0.0.0.0");
            }
            
            hOpenButton = CreateWindowA("BUTTON", "Open", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                      190, 95, 100, 40, hwnd, (HMENU)IDC_OPEN_BUTTON, NULL, NULL);
            
            HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
            SendMessageA(hOpenButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            hMessageEdit = CreateWindowA("EDIT", "", 
                                       WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | 
                                       ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                       20, 155, 440, 215, hwnd, (HMENU)IDC_MESSAGE_EDIT, NULL, NULL);
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_OPEN_BUTTON) {
                StartListening(hwnd);
            }
            break;
        
        case WM_DESTROY:
            StopListening();
            PostQuitMessage(0);
            break;
        
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    FILE *fp = fopen("udp_receiver_config.txt", "r");
    if (fp == NULL) {
        CreateDefaultConfigAndExit();
    }
    fclose(fp);
    
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "UDPReceiverClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);
    
    HWND hwnd = CreateWindowA("UDPReceiverClass", "UDP Receiver",
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT, 500, 420,
                            NULL, NULL, hInstance, NULL);
    
    ShowWindow(hwnd, nCmdShow);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}