#ifndef APISERVICE_H
#define APISERVICE_H

#include "database/DatabaseService.h"
#include "configs/ConfigManager.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define SOCKET_TYPE SOCKET
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #define SOCKET_TYPE int
    #define INVALID_SOCKET_VAL -1
    #define CLOSE_SOCKET close
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

private:
    DatabaseService& dbService;
    ConfigManager configManager;
    ApiConfig apiConfig;
    bool running;
    SOCKET_TYPE serverSocket;
    SOCKET_TYPE shutdownSocket;
    std::thread serverThread;
    std::unordered_map<std::string, Session> sessions;
    std::unordered_map<std::string, PasswordResetToken> passwordResetTokens;
    std::mutex sessionsMutex;
    std::mutex passwordResetMutex;

    void initializeNetwork();
    void cleanupNetwork();
    void runServer();
    void handleClient(SOCKET_TYPE clientSocket);

    // Utility functions
    std::string createJsonResponse(const std::string& content, int statusCode = 200);
    std::string generateSessionToken();
    bool validateSession(const std::string& token);
    std::string getUserIdFromSession(const std::string& token);
    std::string hashPassword(const std::string& password);

    // Auth operations
    std::string handleRegister(const std::string& body);
    std::string handleLogin(const std::string& body);
    std::string handleForgotPassword(const std::string& body);
    std::string handleResetPassword(const std::string& body);
    std::string handleLogout(const std::string& sessionToken);

    // Teacher CRUD operations
    std::string handleAddTeacher(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateTeacher(const std::string& body, int teacherId, const std::string& sessionToken);
    std::string handleDeleteTeacher(int teacherId, const std::string& sessionToken);

    // Student CRUD operations
    std::string handleAddStudent(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateStudent(const std::string& body, int studentId, const std::string& sessionToken);
    std::string handleDeleteStudent(int studentId, const std::string& sessionToken);

    // Group CRUD operations
    std::string handleAddGroup(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateGroup(const std::string& body, int groupId, const std::string& sessionToken);
    std::string handleDeleteGroup(int groupId, const std::string& sessionToken);

    // Portfolio operations
    std::string handleAddPortfolio(const std::string& body, const std::string& sessionToken);

    // Event operations
    std::string handleAddEvent(const std::string& body, const std::string& sessionToken);

    // Profile operations
    std::string handleUpdateProfile(const std::string& body, const std::string& sessionToken);

    // GET operations with token validation
    std::string getTeachersJson(const std::string& sessionToken);
    std::string getStudentsJson(const std::string& sessionToken);
    std::string getGroupsJson(const std::string& sessionToken);
    std::string getPortfolioJson(const std::string& sessionToken);
    std::string getEventsJson(const std::string& sessionToken);
    std::string getSpecializationsJson(const std::string& sessionToken);
    std::string getProfile(const std::string& sessionToken);

    // Status
    std::string handleStatus();
};

#endif // APISERVICE_H