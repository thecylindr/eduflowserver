#include "api/ApiService.h"
#include "json.hpp"
#include <sstream>
#include <regex>
#include <iostream>
#include <iomanip>
#include <openssl/rand.h>  // Добавлен для RAND_bytes

#ifndef _WIN32
    #include <fcntl.h>
    #include <errno.h>
    #include <arpa/inet.h>
#endif

using json = nlohmann::json;

ApiService::ApiService(DatabaseService& dbService) 
    : dbService(dbService), running(false), 
      serverSocket(INVALID_SOCKET_VAL), shutdownSocket(INVALID_SOCKET_VAL) {
    initializeNetwork();
}

ApiService::~ApiService() {
    stop();
    cleanupNetwork();
}

void ApiService::initializeNetwork() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        // WSAStartup failed - ошибка инициализации сети
    }
#endif
}

void ApiService::cleanupNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool ApiService::start() {
    if (running) return true;
    
    if (!configManager.loadApiConfig(apiConfig)) {
        return false;
    }
    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET_VAL) {
        return false;
    }
    
    int opt = 1;
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(serverSocket, FIONBIO, &mode);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    int flags = fcntl(serverSocket, F_GETFL, 0);
    if (flags == -1) {
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    if (fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    
    if (apiConfig.host == "0.0.0.0") {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        serverAddr.sin_addr.s_addr = inet_addr(apiConfig.host.c_str());
        if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
            CLOSE_SOCKET(serverSocket);
            return false;
        }
    }
    
    serverAddr.sin_port = htons(apiConfig.port);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    if (listen(serverSocket, apiConfig.maxConnections) < 0) {
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    running = true;
    serverThread = std::thread(&ApiService::runServer, this);
    
    return true;
}

void ApiService::stop() {
    if (!running) return;
    
    running = false;
    
    shutdownSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (shutdownSocket != INVALID_SOCKET_VAL) {
        sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        
        if (apiConfig.host == "0.0.0.0") {
            serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        } else {
            serverAddr.sin_addr.s_addr = inet_addr(apiConfig.host.c_str());
        }
        
        serverAddr.sin_port = htons(apiConfig.port);
        
        connect(shutdownSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        CLOSE_SOCKET(shutdownSocket);
        shutdownSocket = INVALID_SOCKET_VAL;
    }
    
    if (serverSocket != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VAL;
    }
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void ApiService::runServer() {
    while (running) {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int clientAddrLen = sizeof(clientAddr);
#else
        socklen_t clientAddrLen = sizeof(clientAddr);
#endif
        
        SOCKET_TYPE clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        
        if (!running) break;
        
        if (clientSocket == INVALID_SOCKET_VAL) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                // Accept failed - ошибка принятия соединения
            }
#else
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                // Accept failed - ошибка принятия соединения
            }
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        std::thread clientThread(&ApiService::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void ApiService::handleClient(SOCKET_TYPE clientSocket) {
    char buffer[8192];
    int bytesReceived;
    
#ifdef _WIN32
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
#else
    bytesReceived = read(clientSocket, buffer, sizeof(buffer) - 1);
#endif
    
    if (bytesReceived <= 0) {
        CLOSE_SOCKET(clientSocket);
        return;
    }
    
    buffer[bytesReceived] = '\0';
    std::string request(buffer);
    
    std::string response;
    
    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;
    
    if (method == "OPTIONS") {
        response = createJsonResponse("", 200);
        
#ifdef _WIN32
        send(clientSocket, response.c_str(), response.length(), 0);
#else
        write(clientSocket, response.c_str(), response.length());
#endif
        CLOSE_SOCKET(clientSocket);
        return;
    }
    
    std::unordered_map<std::string, std::string> headers;
    std::string line;
    while (std::getline(iss, line) && line != "\r") {
        size_t pos = line.find(": ");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2);
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            headers[key] = value;
        }
    }
    
    std::string body;
    if (method == "POST" || method == "PUT") {
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            body = request.substr(bodyPos + 4);
        }
    }
    
    std::string sessionToken;
    if (headers.find("Authorization") != headers.end()) {
        std::string authHeader = headers["Authorization"];
        if (authHeader.find("Bearer ") == 0) {
            sessionToken = authHeader.substr(7);
        }
    }
    
    std::regex teacherRegex("^/teachers/(\\d+)$");
    std::regex studentRegex("^/students/(\\d+)$");
    std::regex groupRegex("^/groups/(\\d+)$");
    std::smatch matches;
    
    if (method == "POST" && path == "/register") {
        response = handleRegister(body);
    } else if (method == "POST" && path == "/login") {
        response = handleLogin(body);
    } else if (method == "POST" && path == "/forgot-password") {
        response = handleForgotPassword(body);
    } else if (method == "POST" && path == "/reset-password") {
        response = handleResetPassword(body);
    } else if (method == "POST" && path == "/logout") {
        response = handleLogout(sessionToken);
    } else if (method == "GET" && path == "/api/status") {
        response = handleStatus();
    } else if (method == "GET" && path == "/teachers") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getTeachersJson(sessionToken);
        }
    } else if (method == "POST" && path == "/teachers") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = handleAddTeacher(body, sessionToken);
        }
    } else if (method == "PUT" && std::regex_match(path, matches, teacherRegex)) {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            int teacherId = std::stoi(matches[1]);
            response = handleUpdateTeacher(body, teacherId, sessionToken);
        }
    } else if (method == "DELETE" && std::regex_match(path, matches, teacherRegex)) {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            int teacherId = std::stoi(matches[1]);
            response = handleDeleteTeacher(teacherId, sessionToken);
        }
    } else if (method == "GET" && path == "/students") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getStudentsJson(sessionToken);
        }
    } else if (method == "POST" && path == "/students") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = handleAddStudent(body, sessionToken);
        }
    } else if (method == "PUT" && std::regex_match(path, matches, studentRegex)) {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            int studentId = std::stoi(matches[1]);
            response = handleUpdateStudent(body, studentId, sessionToken);
        }
    } else if (method == "DELETE" && std::regex_match(path, matches, studentRegex)) {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            int studentId = std::stoi(matches[1]);
            response = handleDeleteStudent(studentId, sessionToken);
        }
    } else if (method == "GET" && path == "/groups") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getGroupsJson(sessionToken);
        }
    } else if (method == "POST" && path == "/groups") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = handleAddGroup(body, sessionToken);
        }
    } else if (method == "PUT" && std::regex_match(path, matches, groupRegex)) {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            int groupId = std::stoi(matches[1]);
            response = handleUpdateGroup(body, groupId, sessionToken);
        }
    } else if (method == "DELETE" && std::regex_match(path, matches, groupRegex)) {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            int groupId = std::stoi(matches[1]);
            response = handleDeleteGroup(groupId, sessionToken);
        }
    } else if (method == "GET" && path == "/portfolio") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getPortfolioJson(sessionToken);
        }
    } else if (method == "POST" && path == "/portfolio") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = handleAddPortfolio(body, sessionToken);
        }
    } else if (method == "GET" && path == "/events") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getEventsJson(sessionToken);
        }
    } else if (method == "POST" && path == "/events") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = handleAddEvent(body, sessionToken);
        }
    } else if (method == "GET" && path == "/specializations") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getSpecializationsJson(sessionToken);
        }
    } else if (method == "GET" && path == "/profile") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getProfile(sessionToken);
        }
    } else if (method == "PUT" && path == "/profile") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = handleUpdateProfile(body, sessionToken);
        }
    } else {
        response = createJsonResponse("{\"message\": \"Welcome to EduFlow API!\"}");
    }

#ifdef _WIN32
    send(clientSocket, response.c_str(), response.length(), 0);
#else
    write(clientSocket, response.c_str(), response.length());
#endif
    
    CLOSE_SOCKET(clientSocket);
}

std::string ApiService::createJsonResponse(const std::string& content, int statusCode) {
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 201: statusText = "Created"; break;
        case 400: statusText = "Bad Request"; break;
        case 401: statusText = "Unauthorized"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default: statusText = "OK";
    }
    
    std::stringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
             << "Content-Type: application/json\r\n";
    
    if (apiConfig.enableCors) {
        response << "Access-Control-Allow-Origin: " << apiConfig.corsOrigin << "\r\n"
                 << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                 << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    }
    
    response << "Content-Length: " << content.length() << "\r\n"
             << "\r\n"
             << content;
    return response.str();
}

std::string ApiService::generateSessionToken() {
    unsigned char buffer[32];
    RAND_bytes(buffer, sizeof(buffer));
    
    std::stringstream ss;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i];
    }
    return ss.str();
}

bool ApiService::validateSession(const std::string& token) {
    if (token.empty()) return false;
    
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(token);
    if (it == sessions.end()) return false;
    
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::hours>(now - it->second.createdAt);
    if (duration.count() > apiConfig.sessionTimeoutHours) {
        sessions.erase(it);
        return false;
    }
    
    return true;
}

std::string ApiService::getUserIdFromSession(const std::string& token) {
    if (!validateSession(token)) return "";
    
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(token);
    return it != sessions.end() ? it->second.userId : "";
}