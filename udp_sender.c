#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

#define IDC_IP_EDIT 100
#define IDC_PORT_EDIT 101
#define IDC_MESSAGE_EDIT 102
#define IDC_SEND_BUTTON 103
#define IDC_STATUS_LABEL 104

HWND hIpEdit, hPortEdit, hMessageEdit, hSendButton, hStatusLabel;
SOCKET udpSock = INVALID_SOCKET;

BOOL InitUDP() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) return FALSE;
    
    udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock == INVALID_SOCKET) {
        WSACleanup();
        return FALSE;
    }
    return TRUE;
}

void SendUDPMessage() {
    char message[1024], port[10], ipAddr[64];
    GetWindowTextA(hMessageEdit, message, sizeof(message));
    GetWindowTextA(hPortEdit, port, sizeof(port));
    GetWindowTextA(hIpEdit, ipAddr, sizeof(ipAddr));
    
    if (strlen(message) == 0) {
        SetWindowTextA(hStatusLabel, "Message is empty");
        return;
    }
    
    if (strlen(ipAddr) == 0) {
        SetWindowTextA(hStatusLabel, "IP address is empty");
        return;
    }
    
    int targetPort = atoi(port);
    if (targetPort < 1024 || targetPort > 65535) {
        SetWindowTextA(hStatusLabel, "Port must be 1024-65535");
        return;
    }
    
    struct sockaddr_in destAddr;
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(targetPort);
    destAddr.sin_addr.s_addr = inet_addr(ipAddr);
    
    if (destAddr.sin_addr.s_addr == INADDR_NONE) {
        SetWindowTextA(hStatusLabel, "Invalid IP address");
        return;
    }
    
    int result = sendto(udpSock, message, (int)strlen(message), 0, 
                       (struct sockaddr*)&destAddr, sizeof(destAddr));
    
    if (result == SOCKET_ERROR) {
        SetWindowTextA(hStatusLabel, "Send error");
    } else {
        char status[256];
        sprintf(status, "Sent: %d bytes to %s:%d", result, ipAddr, targetPort);
        SetWindowTextA(hStatusLabel, status);
        SetWindowTextA(hMessageEdit, "");
    }
}

BOOL LoadConfig(char *ipAddr, int *port) {
    FILE *fp = fopen("udp_sender_config.txt", "r");
    if (fp == NULL) {
        return FALSE;
    }
    
    char line1[64], line2[16];
    if (fgets(line1, sizeof(line1), fp) == NULL) {
        fclose(fp);
        return FALSE;
    }
    if (fgets(line2, sizeof(line2), fp) == NULL) {
        fclose(fp);
        return FALSE;
    }
    
    line1[strcspn(line1, "\r\n")] = 0;
    line2[strcspn(line2, "\r\n")] = 0;
    
    int p = atoi(line2);
    if (p < 1024 || p > 65535) {
        fclose(fp);
        return FALSE;
    }
    
    strcpy(ipAddr, line1);
    *port = p;
    fclose(fp);
    return TRUE;
}

void SaveConfig(const char *ipAddr, int port) {
    FILE *fp = fopen("udp_sender_config.txt", "w");
    if (fp != NULL) {
        fprintf(fp, "%s\n%d\n", ipAddr, port);
        fclose(fp);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            CreateWindowA("STATIC", "IP Address:", WS_VISIBLE | WS_CHILD,
                        20, 20, 100, 20, hwnd, NULL, NULL, NULL);
            
            hIpEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                                    130, 18, 200, 25, hwnd, (HMENU)IDC_IP_EDIT, NULL, NULL);
            
            CreateWindowA("STATIC", "Port:", WS_VISIBLE | WS_CHILD,
                        20, 60, 100, 20, hwnd, NULL, NULL, NULL);
            
            hPortEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                                    130, 58, 100, 25, hwnd, (HMENU)IDC_PORT_EDIT, NULL, NULL);
            
            CreateWindowA("STATIC", "Message:", WS_VISIBLE | WS_CHILD,
                        20, 100, 100, 20, hwnd, NULL, NULL, NULL);
            
            hMessageEdit = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                       130, 98, 300, 25, hwnd, (HMENU)IDC_MESSAGE_EDIT, NULL, NULL);
            
            hSendButton = CreateWindowA("BUTTON", "Send", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                      180, 140, 100, 40, hwnd, (HMENU)IDC_SEND_BUTTON, NULL, NULL);
            
            hStatusLabel = CreateWindowA("STATIC", "Ready", WS_VISIBLE | WS_CHILD,
                                       20, 200, 420, 20, hwnd, (HMENU)IDC_STATUS_LABEL, NULL, NULL);
            
            HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
            SendMessageA(hSendButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_SEND_BUTTON) {
                char ipAddr[64], port[10];
                GetWindowTextA(hIpEdit, ipAddr, sizeof(ipAddr));
                GetWindowTextA(hPortEdit, port, sizeof(port));
                SaveConfig(ipAddr, atoi(port));
                SendUDPMessage();
            }
            break;
        
        case WM_DESTROY:
            if (udpSock != INVALID_SOCKET) {
                closesocket(udpSock);
            }
            WSACleanup();
            PostQuitMessage(0);
            break;
        
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    char ipAddr[64];
    int port;
    
    // 設定ファイルの読み込み
    if (!LoadConfig(ipAddr, &port)) {
        SaveConfig("127.0.0.1", 12345);
        MessageBoxA(NULL, "Configuration file created with defaults:\nIP: 127.0.0.1\nPort: 12345\n\nPlease restart the application.", 
                   "First Run", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    
    if (!InitUDP()) {
        MessageBoxA(NULL, "Failed to initialize UDP", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "UDPSenderClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);
    
    HWND hwnd = CreateWindowA("UDPSenderClass", "UDP Sender",
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT, 480, 280,
                            NULL, NULL, hInstance, NULL);
    
    ShowWindow(hwnd, nCmdShow);
    
    char portStr[10];
    sprintf(portStr, "%d", port);
    SetWindowTextA(hIpEdit, ipAddr);
    SetWindowTextA(hPortEdit, portStr);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}