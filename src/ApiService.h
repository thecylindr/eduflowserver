#ifndef APISERVICE_H
#define APISERVICE_H

#include "DatabaseService.h"
#include <thread>
#include <atomic>
#include <string>
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define SOCKET_TYPE SOCKET
    #define CLOSE_SOCKET closesocket
    #define INVALID_SOCKET_VAL INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #define SOCKET_TYPE int
    #define CLOSE_SOCKET close
    #define INVALID_SOCKET_VAL -1
#endif

class ApiService {
public:
    ApiService(DatabaseService& dbService);
    ~ApiService();
    
    bool start();
    void stop();
    bool isRunning() const { return running; }

private:
    DatabaseService& dbService;
    std::thread serverThread;
    std::atomic<bool> running;
    SOCKET_TYPE serverSocket;
    
    void runServer();
    void handleClient(SOCKET_TYPE clientSocket);
    std::string createJsonResponse(const std::string& content);
    std::string getStudentsJson();
    std::string getTeachersJson();
    std::string getGroupsJson();
    
    void initializeNetwork();
    void cleanupNetwork();
    
    // Добавляем shutdown сокет для корректной остановки
    SOCKET_TYPE shutdownSocket;
};

#endif