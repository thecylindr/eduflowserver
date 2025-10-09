#ifndef APISERVICE_H
#define APISERVICE_H

#include "database/DatabaseService.h"
#include <thread>
#include <atomic>
#include <string>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <sstream>
#include <iomanip>

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
    #include <cstring>
    #define SOCKET_TYPE int
    #define CLOSE_SOCKET close
    #define INVALID_SOCKET_VAL -1
#endif

struct Session {
    std::string userId;
    std::string email;
    std::chrono::system_clock::time_point createdAt;
};

struct PasswordResetToken {
    std::string email;
    std::chrono::system_clock::time_point createdAt;
};

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
    SOCKET_TYPE shutdownSocket;
    
    // Session management
    std::unordered_map<std::string, Session> sessions;
    std::unordered_map<std::string, PasswordResetToken> passwordResetTokens;
    std::mutex sessionsMutex;
    std::mutex passwordResetMutex;
    
    void runServer();
    void handleClient(SOCKET_TYPE clientSocket);
    std::string createJsonResponse(const std::string& content, int statusCode = 200);
    std::string getStudentsJson();
    std::string getTeachersJson();
    std::string getGroupsJson();
    
    // Authentication methods
    std::string hashPassword(const std::string& password);
    std::string generateSessionToken();
    bool validateSession(const std::string& token);
    std::string getUserIdFromSession(const std::string& token);
    
    // HTTP handlers
    std::string handleRegister(const std::string& body);
    std::string handleLogin(const std::string& body);
    std::string handleForgotPassword(const std::string& body);
    std::string handleResetPassword(const std::string& body);
    std::string handleLogout(const std::string& sessionToken);
    std::string getProfile(const std::string& sessionToken);
    
    void initializeNetwork();
    void cleanupNetwork();
};

#endif