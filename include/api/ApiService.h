#ifndef APISERVICE_H
#define APISERVICE_H

#include "database/DatabaseService.h"
#include "configs/ConfigManager.h"
#include "models/Models.h"
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>
#include <random>
#include <string>
#include <memory>
#include <atomic>

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

class ApiService {
private:
    DatabaseService& dbService;
    ConfigManager configManager;
    ApiConfig apiConfig;
    std::unordered_map<std::string, Session> sessions;
    std::unordered_map<std::string, PasswordResetToken> passwordResetTokens;
    std::mutex sessionsMutex;
    std::mutex passwordResetMutex;
    std::atomic<bool> running;
    SOCKET_TYPE serverSocket;
    std::thread serverThread;
    std::thread cleanupThread;
    
    std::string getClientInfo(SOCKET_TYPE clientSocket);
    void initializeNetwork();
    void cleanupNetwork();
    void runServer();
    void handleClient(SOCKET_TYPE clientSocket);
    void runCleanup();
    std::string processRequest(const std::string& method, const std::string& path,
                               const std::string& body, const std::string& sessionToken,
                               const std::string& clientInfo = "");
    std::string createJsonResponse(const std::string& content, int statusCode = 200);
    std::string generateSessionToken();
    
    // Session management
    void cleanupExpiredSessions();
    void loadSessionsFromDB();
    bool validateTokenInDatabase(const std::string& token); // Новая функция для проверки токена в БД

public:
    ApiService(DatabaseService& dbService);
    ~ApiService();
    bool start();
    void stop();
    std::string processRequestFromRaw(const std::string& rawRequest, const std::string& clientIP = "");
    std::string handleRevokeSessionByToken(const std::string& targetToken, const std::string& sessionToken);
    
    // Authentication
    std::string handleRegister(const std::string& body, const std::string& clientInfo = "");
    std::string handleLogin(const std::string& body, const std::string& clientInfo = "");
    std::string handleForgotPassword(const std::string& body);
    std::string handleResetPassword(const std::string& body);
    std::string handleLogout(const std::string& sessionToken, const std::string& clientInfo = "");
    
    // Profile and password management
    std::string handleChangePassword(const std::string& body, const std::string& sessionToken);
    std::string getSessionInfo(const std::string& token);
    std::string handleGetSessions(const std::string& sessionToken);
    std::string handleRevokeSession(const std::string& body, const std::string& sessionToken);
    
    // Session validation
    bool validateSession(const std::string& token);
    std::string getUserIdFromSession(const std::string& token);
    
    // Password hashing
    std::string hashPassword(const std::string& password);
    
    // Specialization management
    std::string handleAddSpecialization(const std::string& body, const std::string& sessionToken);
    std::string handleDeleteSpecialization(int specializationCode, const std::string& sessionToken);
    
    // Teacher specialization management
    std::string handleAddTeacherSpecialization(const std::string& body, const std::string& sessionToken);
    std::string handleRemoveTeacherSpecialization(int teacherId, int specializationCode, const std::string& sessionToken);
    std::string getTeacherSpecializationsJson(int teacherId, const std::string& sessionToken);
    
    // CRUD operations
    std::string handleAddTeacher(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateTeacher(const std::string& body, int teacherId, const std::string& sessionToken);
    std::string handleDeleteTeacher(int teacherId, const std::string& sessionToken);
    std::string handleAddStudent(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateStudent(const std::string& body, int studentId, const std::string& sessionToken);
    std::string handleDeleteStudent(int studentId, const std::string& sessionToken);
    std::string handleAddGroup(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateGroup(const std::string& body, int groupId, const std::string& sessionToken);
    std::string handleDeleteGroup(int groupId, const std::string& sessionToken);
    std::string handleUpdateProfile(const std::string& body, const std::string& sessionToken);
    
    // Getters
    std::string getProfile(const std::string& sessionToken);
    std::string getTeachersJson(const std::string& sessionToken);
    std::string getStudentsJson(const std::string& sessionToken);
    std::string getGroupsJson(const std::string& sessionToken);
    std::string getSpecializationsJson(const std::string& sessionToken);
    std::string handleStatus();

    // Portfolio
    std::string handleAddPortfolio(const std::string& body, const std::string& sessionToken);
    std::string handleUpdatePortfolio(const std::string& body, int portfolioId, const std::string& sessionToken);
    std::string handleDeletePortfolio(int portfolioId, const std::string& sessionToken);
    std::string getPortfolioJson(const std::string& sessionToken);

    // Event
    std::string handleAddEvent(const std::string& body, const std::string& sessionToken);
    std::string handleUpdateEvent(const std::string& body, int eventId, const std::string& sessionToken);
    std::string handleDeleteEvent(int eventId, const std::string& sessionToken);
    std::string getEventsJson(const std::string& sessionToken);

    std::string handleAddEventCategory(const std::string& body, const std::string& sessionToken);

    std::string getEventCategoriesJson(const std::string& sessionToken);
    std::string handleUpdateEventCategory(const std::string& body, const std::string& eventType, const std::string& sessionToken);
    std::string handleDeleteEventCategory(int eventCode, const std::string& sessionToken);
    std::string handleUpdateEventCategory(const std::string& body, int categoryId, const std::string& sessionToken);

};

#endif