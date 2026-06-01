#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const char* INI_FILE = ".\\fm_UDP_to_TCP_config.ini";
const char* SECTION = "Setting";
const int MAX_CLIENTS = 64;
const int RECONNECT_INTERVAL_MS = 5000;

struct Config {
    int tcpListenPort;
    int udpListenPort;
    int udpBroadcastPort;
    string udpBroadcastIP;
};

bool LoadConfig(Config& config) {
    DWORD attr = GetFileAttributesA(INI_FILE);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    
    config.tcpListenPort = GetPrivateProfileIntA(SECTION, "tcp_listen_port", 0, INI_FILE);
    if (config.tcpListenPort == 0) return false;
    
    config.udpListenPort = GetPrivateProfileIntA(SECTION, "udp_listen_port", 0, INI_FILE);
    if (config.udpListenPort == 0) return false;
    
    config.udpBroadcastPort = GetPrivateProfileIntA(SECTION, "udp_broadcast_port", 0, INI_FILE);
    if (config.udpBroadcastPort == 0) return false;
    
    char ipBuffer[256];
    GetPrivateProfileStringA(SECTION, "udp_broadcast_ipaddr", "", ipBuffer, sizeof(ipBuffer), INI_FILE);
    config.udpBroadcastIP = ipBuffer;
    if (config.udpBroadcastIP.empty()) return false;
    
    return true;
}

void SaveDefaultConfig() {
    WritePrivateProfileStringA(SECTION, "tcp_listen_port", "51234", INI_FILE);
    WritePrivateProfileStringA(SECTION, "udp_listen_port", "41449", INI_FILE);
    WritePrivateProfileStringA(SECTION, "udp_broadcast_port", "12345", INI_FILE);
    WritePrivateProfileStringA(SECTION, "udp_broadcast_ipaddr", "192.168.1.255", INI_FILE);
}

int main() {
    Config config;
    
    if (!LoadConfig(config)) {
        cout << "[System] Configuration file not found or invalid." << endl;
        cout << "[System] Creating default config file: fm_UDP_to_TCP_config.ini" << endl;
        SaveDefaultConfig();
        cout << "[System] Please edit the configuration file and run again." << endl;
        cout << "\nConfiguration format (fm_UDP_to_TCP_config.ini):" << endl;
        cout << "[Setting]" << endl;
        cout << "tcp_listen_port=51234" << endl;
        cout << "udp_listen_port=41449" << endl;
        cout << "udp_broadcast_port=12345" << endl;
        cout << "udp_broadcast_ipaddr=192.168.1.255" << endl;
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
    cout << "  TCP listen port: " << config.tcpListenPort << endl;
    cout << "  UDP listen port: " << config.udpListenPort << endl;
    cout << "  UDP broadcast port: " << config.udpBroadcastPort << endl;
    cout << "  UDP broadcast IP: " << config.udpBroadcastIP << endl;
    cout << "  Max TCP clients: " << MAX_CLIENTS << endl;
    cout << endl;

    SOCKET udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock == INVALID_SOCKET) {
        cout << "[Error] Failed to create UDP listen socket" << endl;
        system("pause");
        WSACleanup();
        return 1;
    }

    sockaddr_in udpAddr = {};
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port = htons(config.udpListenPort);
    udpAddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(udpSock, (sockaddr*)&udpAddr, sizeof(udpAddr)) == SOCKET_ERROR) {
        cout << "[Error] Failed to bind UDP port " << config.udpListenPort << endl;
        cout << "Error code: " << WSAGetLastError() << endl;
        closesocket(udpSock);
        system("pause");
        WSACleanup();
        return 1;
    }

    cout << "[System] UDP listening on port " << config.udpListenPort << endl;

    SOCKET udpBroadcastSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpBroadcastSock == INVALID_SOCKET) {
        cout << "[Error] Failed to create UDP broadcast socket" << endl;
        closesocket(udpSock);
        system("pause");
        WSACleanup();
        return 1;
    }

    int broadcastEnable = 1;
    if (setsockopt(udpBroadcastSock, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable)) == SOCKET_ERROR) {
        cout << "[Warning] Failed to enable SO_BROADCAST. Error code: " << WSAGetLastError() << endl;
    }

    sockaddr_in broadcastAddr = {};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(config.udpBroadcastPort);
    inet_pton(AF_INET, config.udpBroadcastIP.c_str(), &broadcastAddr.sin_addr);

    cout << "[System] UDP broadcast configured to " << config.udpBroadcastIP 
         << ":" << config.udpBroadcastPort << endl;

    SOCKET tcpListenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpListenSock == INVALID_SOCKET) {
        cout << "[Error] Failed to create TCP socket" << endl;
        closesocket(udpSock);
        closesocket(udpBroadcastSock);
        system("pause");
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(tcpListenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in tcpAddr = {};
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_port = htons(config.tcpListenPort);
    tcpAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(tcpListenSock, (sockaddr*)&tcpAddr, sizeof(tcpAddr)) == SOCKET_ERROR) {
        cout << "[Error] Failed to bind TCP port " << config.tcpListenPort << endl;
        cout << "Error code: " << WSAGetLastError() << endl;
        closesocket(udpSock);
        closesocket(udpBroadcastSock);
        closesocket(tcpListenSock);
        system("pause");
        WSACleanup();
        return 1;
    }

    if (listen(tcpListenSock, SOMAXCONN) == SOCKET_ERROR) {
        cout << "[Error] Failed to listen on TCP port" << endl;
        cout << "Error code: " << WSAGetLastError() << endl;
        closesocket(udpSock);
        closesocket(udpBroadcastSock);
        closesocket(tcpListenSock);
        system("pause");
        WSACleanup();
        return 1;
    }

    cout << "[System] TCP server listening on port " << config.tcpListenPort << endl;
    cout << "[System] Waiting for TCP client connections..." << endl;
    cout << "[System] Ready to receive UDP messages\n" << endl;

    vector<SOCKET> clientSockets;
    
    SOCKET loopbackClientSock = INVALID_SOCKET;
    bool loopbackConnected = false;
    DWORD lastReconnectAttempt = 0;

    char buffer[1024];
    sockaddr_in senderAddr;
    int senderLen = sizeof(senderAddr);

    while (true) {
        DWORD currentTime = GetTickCount();
        if (loopbackClientSock == INVALID_SOCKET && 
            (currentTime - lastReconnectAttempt) >= RECONNECT_INTERVAL_MS) {
            
            lastReconnectAttempt = currentTime;
            
            loopbackClientSock = socket(AF_INET, SOCK_STREAM, 0);
            if (loopbackClientSock != INVALID_SOCKET) {
                u_long mode = 1;
                ioctlsocket(loopbackClientSock, FIONBIO, &mode);
                
                sockaddr_in loopbackAddr = {};
                loopbackAddr.sin_family = AF_INET;
                loopbackAddr.sin_port = htons(config.tcpListenPort);
                inet_pton(AF_INET, "127.0.0.1", &loopbackAddr.sin_addr);
                
                int connectResult = connect(loopbackClientSock, (sockaddr*)&loopbackAddr, sizeof(loopbackAddr));
                if (connectResult == SOCKET_ERROR) {
                    int error = WSAGetLastError();
                    if (error == WSAEWOULDBLOCK) {
                        cout << "[Loopback] Connecting to 127.0.0.1:" << config.tcpListenPort << "..." << endl;
                        loopbackConnected = false;
                    } else {
                        cout << "[Loopback] Connection failed. Error code: " << error << endl;
                        closesocket(loopbackClientSock);
                        loopbackClientSock = INVALID_SOCKET;
                    }
                } else {
                    loopbackConnected = true;
                    cout << "[Loopback] Connected to 127.0.0.1:" << config.tcpListenPort << endl;
                }
            }
        }

        fd_set readSet, writeSet;
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        
        FD_SET(tcpListenSock, &readSet);
        
        FD_SET(udpSock, &readSet);
        
        for (size_t i = 0; i < clientSockets.size(); i++) {
            FD_SET(clientSockets[i], &readSet);
        }
        
        if (loopbackClientSock != INVALID_SOCKET) {
            if (loopbackConnected) {
                FD_SET(loopbackClientSock, &readSet);
            } else {
                FD_SET(loopbackClientSock, &writeSet);
            }
        }
        
        timeval timeout = {0, 100000};
        int activity = select(0, &readSet, &writeSet, NULL, &timeout);
        
        if (activity == SOCKET_ERROR) {
            cout << "[Error] select() failed. Error code: " << WSAGetLastError() << endl;
            Sleep(100);
            continue;
        }
        
        if (loopbackClientSock != INVALID_SOCKET && !loopbackConnected && FD_ISSET(loopbackClientSock, &writeSet)) {
            int error = 0;
            int len = sizeof(error);
            if (getsockopt(loopbackClientSock, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0 && error == 0) {
                loopbackConnected = true;
                cout << "[Loopback] Successfully connected to 127.0.0.1:" << config.tcpListenPort << endl;
            } else {
                cout << "[Loopback] Connection failed. Error code: " << error << endl;
                closesocket(loopbackClientSock);
                loopbackClientSock = INVALID_SOCKET;
            }
        }
        
        if (loopbackClientSock != INVALID_SOCKET && loopbackConnected && FD_ISSET(loopbackClientSock, &readSet)) {
            int recvLen = recv(loopbackClientSock, buffer, sizeof(buffer) - 1, 0);
            
            if (recvLen > 0) {
                buffer[recvLen] = '\0';
                
                cout << "[Loopback] Received " << recvLen << " bytes from TCP" << endl;
                
                int sentLen = sendto(udpBroadcastSock, buffer, recvLen, 0, 
                                    (sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
                
                if (sentLen > 0) {
                    cout << "[UDP Broadcast] Sent " << sentLen << " bytes to " 
                         << config.udpBroadcastIP << ":" << config.udpBroadcastPort << endl;
                } else {
                    cout << "[Warning] UDP broadcast failed. Error code: " << WSAGetLastError() << endl;
                }
            } else {
                cout << "[Loopback] Connection closed. Will retry..." << endl;
                closesocket(loopbackClientSock);
                loopbackClientSock = INVALID_SOCKET;
                loopbackConnected = false;
            }
        }
        
        if (FD_ISSET(tcpListenSock, &readSet)) {
            sockaddr_in clientAddr;
            int clientLen = sizeof(clientAddr);
            SOCKET newClientSock = accept(tcpListenSock, (sockaddr*)&clientAddr, &clientLen);
            
            if (newClientSock != INVALID_SOCKET) {
                if (clientSockets.size() < MAX_CLIENTS) {
                    clientSockets.push_back(newClientSock);
                    
                    char clientIP[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
                    cout << "[System] TCP client connected from " << clientIP << ":" 
                         << ntohs(clientAddr.sin_port) 
                         << " (Total clients: " << clientSockets.size() << ")" << endl;
                } else {
                    char clientIP[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
                    cout << "[Warning] Max clients (" << MAX_CLIENTS << ") reached. "
                         << "Rejecting connection from " << clientIP << ":" 
                         << ntohs(clientAddr.sin_port) << endl;
                    closesocket(newClientSock);
                }
            }
        }
        
        if (FD_ISSET(udpSock, &readSet)) {
            int recvLen = recvfrom(udpSock, buffer, sizeof(buffer) - 1, 0, 
                                  (sockaddr*)&senderAddr, &senderLen);
            
            if (recvLen > 0) {
                buffer[recvLen] = '\0';
                
                char senderIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(senderAddr.sin_addr), senderIP, INET_ADDRSTRLEN);
                
                cout << "[UDP Received from " << senderIP << ":" 
                     << ntohs(senderAddr.sin_port) << "] " << buffer << endl;
                
                if (clientSockets.size() > 0) {
                    cout << "[Broadcasting to " << clientSockets.size() << " client(s)]" << endl;
                    
                    vector<size_t> disconnectedIndices;
                    
                    for (size_t i = 0; i < clientSockets.size(); i++) {
                        int sentLen = send(clientSockets[i], buffer, recvLen, 0);
                        
                        if (sentLen == SOCKET_ERROR) {
                            cout << "[Warning] Failed to send to client #" << i 
                                 << ". Marking for disconnection." << endl;
                            disconnectedIndices.push_back(i);
                        } else {
                            cout << "  -> Client #" << i << ": " << sentLen << " bytes sent" << endl;
                        }
                    }
                    
                    for (int i = disconnectedIndices.size() - 1; i >= 0; i--) {
                        size_t idx = disconnectedIndices[i];
                        closesocket(clientSockets[idx]);
                        clientSockets.erase(clientSockets.begin() + idx);
                        cout << "[System] Client #" << idx << " disconnected and removed. "
                             << "(Remaining clients: " << clientSockets.size() << ")" << endl;
                    }
                    
                    cout << endl;
                } else {
                    cout << "[Info] No TCP clients connected. UDP data not forwarded.\n" << endl;
                }
            } else if (recvLen == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK) {
                    cout << "[Warning] UDP receive error. Error code: " << error << endl;
                }
            }
        }
        
        vector<size_t> disconnectedIndices;
        
        for (size_t i = 0; i < clientSockets.size(); i++) {
            if (FD_ISSET(clientSockets[i], &readSet)) {
                char testBuf[1];
                int recvResult = recv(clientSockets[i], testBuf, sizeof(testBuf), MSG_PEEK);
                
                if (recvResult == 0 || recvResult == SOCKET_ERROR) {
                    disconnectedIndices.push_back(i);
                }
            }
        }
        
        for (int i = disconnectedIndices.size() - 1; i >= 0; i--) {
            size_t idx = disconnectedIndices[i];
            cout << "[System] Client #" << idx << " disconnected." << endl;
            closesocket(clientSockets[idx]);
            clientSockets.erase(clientSockets.begin() + idx);
            cout << "[System] Client removed. (Remaining clients: " 
                 << clientSockets.size() << ")" << endl;
        }
    }

    if (loopbackClientSock != INVALID_SOCKET) {
        closesocket(loopbackClientSock);
    }
    for (size_t i = 0; i < clientSockets.size(); i++) {
        closesocket(clientSockets[i]);
    }
    closesocket(tcpListenSock);
    closesocket(udpSock);
    closesocket(udpBroadcastSock);
    WSACleanup();
    
    return 0;
}