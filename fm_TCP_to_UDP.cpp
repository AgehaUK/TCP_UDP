#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const char* INI_FILE = ".\\fm_TCP_to_UDP_config.ini";
const char* SECTION = "Setting";
const int RECONNECT_INTERVAL_MS = 10000;

struct Config {
    int udpPort;
    int tcpPort;
    string tcpIPAddr;
    string udpIPAddr;
};

bool LoadConfig(Config& config) {
    char buffer[256];
    
    DWORD attr = GetFileAttributesA(INI_FILE);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    
    // UDP port
    config.udpPort = GetPrivateProfileIntA(SECTION, "udp_port", 0, INI_FILE);
    if (config.udpPort == 0) return false;
    
    // TCP port
    config.tcpPort = GetPrivateProfileIntA(SECTION, "tcp_port", 0, INI_FILE);
    if (config.tcpPort == 0) return false;
    
    // UDP IP
    GetPrivateProfileStringA(SECTION, "udp_ipaddr", "", buffer, sizeof(buffer), INI_FILE);
    if (strlen(buffer) == 0) return false;
    config.udpIPAddr = buffer;
    
    // TCP IP
    GetPrivateProfileStringA(SECTION, "tcp_ipaddr", "", buffer, sizeof(buffer), INI_FILE);
    if (strlen(buffer) == 0) return false;
    config.tcpIPAddr = buffer;
    
    return true;
}

void SaveDefaultConfig() {
    WritePrivateProfileStringA(SECTION, "udp_port", "41449", INI_FILE);
    WritePrivateProfileStringA(SECTION, "tcp_port", "51234", INI_FILE);
    WritePrivateProfileStringA(SECTION, "udp_ipaddr", "192.168.101.255", INI_FILE);
    WritePrivateProfileStringA(SECTION, "tcp_ipaddr", "192.168.100.18", INI_FILE);
}

SOCKET CreateAndConnectTCP(const Config& config) {
    SOCKET tcpSock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSock == INVALID_SOCKET) {
        cout << "[Error] Failed to create TCP socket" << endl;
        return INVALID_SOCKET;
    }

    sockaddr_in tcpAddr = {};
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_port = htons(config.tcpPort);
    tcpAddr.sin_addr.s_addr = inet_addr(config.tcpIPAddr.c_str());

    cout << "[System] Connecting to TCP server " << config.tcpIPAddr << ":" 
         << config.tcpPort << "..." << endl;
    
    if (connect(tcpSock, (sockaddr*)&tcpAddr, sizeof(tcpAddr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        cout << "[Warning] Failed to connect to TCP server. Error code: " << error << endl;
        closesocket(tcpSock);
        return INVALID_SOCKET;
    }

    cout << "[System] Connected to TCP server" << endl;
    return tcpSock;
}

int main() {
    Config config;
    
    if (!LoadConfig(config)) {
        cout << "[System] Configuration file not found or invalid." << endl;
        cout << "[System] Creating default config file: fm_TCP_to_UDP_config.ini" << endl;
        SaveDefaultConfig();
        cout << "[System] Please edit the configuration file and run again." << endl;
        cout << "\nConfiguration format (fm_TCP_to_UDP_config.ini):" << endl;
        cout << "[Setting]" << endl;
        cout << "udp_port=41449" << endl;
        cout << "tcp_port=51234" << endl;
        cout << "udp_ipaddr=192.168.101.255" << endl;
        cout << "tcp_ipaddr=192.168.100.18" << endl;
        cout << "\nPress Enter to exit...";
        cin.get();
        return 0;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        cout << "[Error] Failed to initialize Winsock" << endl;
        system("pause");
        return 1;
    }

    cout << "[System] Configuration loaded:" << endl;
    cout << "  TCP server: " << config.tcpIPAddr << ":" << config.tcpPort << endl;
    cout << "  UDP broadcast: " << config.udpIPAddr << ":" << config.udpPort << endl;
    cout << "  Reconnect interval: " << (RECONNECT_INTERVAL_MS / 1000) << " seconds" << endl;
    cout << endl;

    SOCKET udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock == INVALID_SOCKET) {
        cout << "[Error] Failed to create UDP socket" << endl;
        system("pause");
        WSACleanup();
        return 1;
    }

    int opt = 1;
    if (setsockopt(udpSock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        cout << "[Warning] Failed to enable broadcast. Error code: " << WSAGetLastError() << endl;
    }

    sockaddr_in udpAddr = {};
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port = htons(config.udpPort);
    udpAddr.sin_addr.s_addr = inet_addr(config.udpIPAddr.c_str());

    cout << "[System] UDP broadcast destination set to " << config.udpIPAddr 
         << ":" << config.udpPort << endl;

    char buffer[1024];
    SOCKET tcpSock = INVALID_SOCKET;
    DWORD lastConnectAttempt = 0;
    bool firstConnectionAttempt = true;

    while (true) {
        DWORD currentTime = GetTickCount();
        if (tcpSock == INVALID_SOCKET) {
            if ((currentTime - lastConnectAttempt) >= RECONNECT_INTERVAL_MS) {
                lastConnectAttempt = currentTime;
                tcpSock = CreateAndConnectTCP(config);
                
                if (tcpSock != INVALID_SOCKET) {
                    cout << "[System] Waiting for TCP messages...\n" << endl;
                    firstConnectionAttempt = false;
                } else if (firstConnectionAttempt) {
                    cout << "[System] Will retry in " << (RECONNECT_INTERVAL_MS / 1000) 
                         << " seconds...\n" << endl;
                    firstConnectionAttempt = false;
                }
            }
            Sleep(100);
            continue;
        }

        u_long mode = 1;
        ioctlsocket(tcpSock, FIONBIO, &mode);

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(tcpSock, &readSet);
        
        timeval timeout = {0, 100000};
        int selectResult = select(0, &readSet, NULL, NULL, &timeout);
        
        if (selectResult == SOCKET_ERROR) {
            cout << "[Error] select() failed. Error code: " << WSAGetLastError() << endl;
            cout << "[System] Connection lost. Reconnecting..." << endl;
            closesocket(tcpSock);
            tcpSock = INVALID_SOCKET;
            lastConnectAttempt = GetTickCount() - RECONNECT_INTERVAL_MS;
            continue;
        }
        
        if (selectResult == 0) {
            continue;
        }

        mode = 0;
        ioctlsocket(tcpSock, FIONBIO, &mode);

        int recvLen = recv(tcpSock, buffer, sizeof(buffer) - 1, 0);
        
        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            
            cout << "[TCP Received from " << config.tcpIPAddr << ":" 
                 << config.tcpPort << "] " << buffer << endl;
            
            // Send to UDP
            int sentLen = sendto(udpSock, buffer, recvLen, 0,
                               (sockaddr*)&udpAddr, sizeof(udpAddr));
            if (sentLen > 0) {
                cout << "[UDP Broadcast] " << sentLen << " bytes to " 
                     << config.udpIPAddr << ":" << config.udpPort << endl << endl;
            } else {
                cout << "[Error] Failed to send UDP broadcast" << endl;
                cout << "Error code: " << WSAGetLastError() << endl << endl;
            }
        } else if (recvLen == 0) {
            cout << "[System] TCP server disconnected" << endl;
            cout << "[System] Will attempt to reconnect...\n" << endl;
            closesocket(tcpSock);
            tcpSock = INVALID_SOCKET;
            lastConnectAttempt = GetTickCount() - RECONNECT_INTERVAL_MS;
        } else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                cout << "[Error] TCP receive error. Error code: " << error << endl;
                cout << "[System] Connection lost. Reconnecting...\n" << endl;
                closesocket(tcpSock);
                tcpSock = INVALID_SOCKET;
                lastConnectAttempt = GetTickCount() - RECONNECT_INTERVAL_MS;
            }
        }
    }

    if (tcpSock != INVALID_SOCKET) {
        closesocket(tcpSock);
    }
    closesocket(udpSock);
    WSACleanup();
    
    return 0;
}