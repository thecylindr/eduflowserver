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

#include "article/ArticleEditor.h"

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

    ArticleEditor articleEditor;
    
    std::string getClientInfo(SOCKET_TYPE clientSocket);
    void initializeNetwork();
    void cleanupNetwork();
    void runServer();
    void handleClient(SOCKET_TYPE clientSocket);
    void runCleanup();
    std::string processRequest(const std::string& method, const std::string& path,
                                const std::string& body,
                                const std::string& sessionToken,
                                const std::string& clientInfo = "");
    std::string createJsonResponse(const std::string& content, int statusCode = 200);
    std::string generateSessionToken();
    
    // Session management
    void cleanupExpiredSessions();
    void loadSessionsFromDB();
    bool validateTokenInDatabase(const std::string& token);

public:
    ApiService(DatabaseService& dbService);
    ~ApiService();
    bool start();
    void stop();
    std::string handleGetEditor() {
    return createJsonResponse("{\"success\": true, \"message\": \"Editor endpoint\"}");
}

    std::string processRequestFromRaw(const std::string& rawRequest, const std::string& clientIP = "");
    std::string handleRevokeSessionByToken(const std::string& targetToken, const std::string& sessionToken);
    
    std::string handleGetDashboard(const std::string& sessionToken);

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
    std::string handleAddSpecialization(const std::string& body);
    std::string handleDeleteSpecialization(int specializationCode);
    
    // Teacher specialization management
    std::string handleAddTeacherSpecialization(const std::string& body);
    std::string handleRemoveTeacherSpecialization(int teacherId, int specializationCode);
    std::string getTeacherSpecializationsJson(int teacherId);
    
    // CRUD operations
    std::string handleAddTeacher(const std::string& body);
    std::string handleUpdateTeacher(const std::string& body, int teacherId);
    std::string handleDeleteTeacher(int teacherId);
    std::string handleAddStudent(const std::string& body);
    std::string handleUpdateStudent(const std::string& body, int studentId);
    std::string handleDeleteStudent(int studentId);
    std::string handleAddGroup(const std::string& body);
    std::string handleUpdateGroup(const std::string& body, int groupId);
    std::string handleDeleteGroup(int groupId);
    std::string handleGetStudentsByGroup(int groupId);
    std::string handleUpdateProfile(const std::string& body, const std::string& sessionToken);
    
    // Getters
    std::string getProfile(const std::string& sessionToken);
    std::string getTeachersJson(const std::string& sessionToken);
    std::string getStudentsJson(const std::string& sessionToken);
    std::string getGroupsJson(const std::string& sessionToken);
    std::string getSpecializationsJson(const std::string& sessionToken);
    std::string handleStatus();

    // Portfolio
    std::string handleAddPortfolio(const std::string& body);
    std::string handleUpdatePortfolio(const std::string& body, int portfolioId);
    std::string handleDeletePortfolio(int portfolioId);
    std::string getPortfolioJson(const std::string& sessionToken);

    // Event
    std::string handleAddEvent(const std::string& body);
    std::string handleUpdateEvent(const std::string& body, int eventId);
    std::string handleDeleteEvent(int eventId);
    std::string getEventsJson(const std::string& sessionToken);

    std::string handleAddEventCategory(const std::string& body);

    std::string getEventCategoriesJson();
    std::string handleUpdateEventCategory(const std::string& body, const std::string& eventType);
    std::string handleDeleteEventCategory(int eventCode);
    std::string handleUpdateEventCategory(const std::string& body, int categoryId);

    // News methods
    bool isSafeNewsFilename(const std::string& filename);
    std::string handleGetNewsList();
    std::string handleGetNews(const std::string& filename);


};

#endif