#include "api/ApiService.h"
#include "json.hpp"
#include <sstream>
#include <regex>
#include <iostream>
#include <iomanip>
#include <openssl/rand.h>
#include <fstream>
#include <algorithm>
#include <random>
#include <atomic>

#ifndef _WIN32
    #include <fcntl.h>
    #include <errno.h>
    #include <arpa/inet.h>
#endif

using json = nlohmann::json;

ApiService::ApiService(DatabaseService& dbService) 
    : dbService(dbService), 
      running(false),
      serverSocket(INVALID_SOCKET_VAL) {
    std::cout << "🔧 Initializing ApiService..." << std::endl;
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
        std::cout << "WSAStartup failed with error: " << result << std::endl;
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
        std::cout << "Failed to load API config" << std::endl;
        return false;
    }
    
    loadSessionsFromFile();
    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET_VAL) {
        std::cout << "Failed to create server socket" << std::endl;
        return false;
    }
    
    int opt = 1;
#ifdef _WIN32
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cout << "Failed to set socket options" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    u_long mode = 1;
    if (ioctlsocket(serverSocket, FIONBIO, &mode) != 0) {
        std::cout << "Failed to set non-blocking mode" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
#else
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cout << "Failed to set socket options" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    int flags = fcntl(serverSocket, F_GETFL, 0);
    if (flags == -1) {
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    if (fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
        CLOSE_SOCKET(serverSocket);
        return false;
    }
#endif
    
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    
    if (apiConfig.host == "0.0.0.0") {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        serverAddr.sin_addr.s_addr = inet_addr(apiConfig.host.c_str());
        if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
            std::cout << "Invalid host address: " << apiConfig.host << std::endl;
            CLOSE_SOCKET(serverSocket);
            return false;
        }
    }
    
    serverAddr.sin_port = htons(apiConfig.port);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cout << "Failed to bind socket to " << apiConfig.host << ":" << apiConfig.port << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    if (listen(serverSocket, apiConfig.maxConnections) < 0) {
        std::cout << "Failed to listen on socket" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    running = true;
    serverThread = std::thread(&ApiService::runServer, this);
    cleanupThread = std::thread(&ApiService::runCleanup, this);
    
    std::cout << "API Server started on " << apiConfig.host << ":" << apiConfig.port << std::endl;
    std::cout << "Session timeout: " << apiConfig.sessionTimeoutHours << " hours" << std::endl;
    
    return true;
}

void ApiService::runCleanup() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::hours(1));
        cleanupExpiredSessions();
        saveSessionsToFile();
    }
}

void ApiService::stop() {
    if (!running) return;
    
    running = false;
    
    // Сохраняем сессии перед остановкой
    saveSessionsToFile();
    
    // Останавливаем cleanup thread
    if (cleanupThread.joinable()) {
        cleanupThread.join();
    }
    
    // Закрываем серверный сокет для выхода из accept()
    if (serverSocket != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VAL;
    }
    
    // Ждем завершения серверного потока
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    std::cout << "API Server stopped" << std::endl;
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
            if (error != WSAEWOULDBLOCK && error != WSAECONNRESET) {
                std::cout << "Accept failed with error: " << error << std::endl;
            }
#else
            if (errno != EWOULDBLOCK && errno != EAGAIN && errno != ECONNABORTED) {
                std::cout << "Accept failed with error: " << errno << std::endl;
            }
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Обрабатываем клиента в отдельном потоке
        std::thread clientThread(&ApiService::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void ApiService::handleClient(SOCKET_TYPE clientSocket) {
    char buffer[16384] = {0};
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
    
    // Гарантируем нуль-терминатор
    buffer[bytesReceived] = '\0';
    
    std::string request(buffer, bytesReceived);
    std::string response;
    
    try {
        std::istringstream iss(request);
        std::string method, path, protocol;
        iss >> method >> path >> protocol;
        
        // Безопасная обработка OPTIONS запроса
        if (method == "OPTIONS") {
            response = "HTTP/1.1 200 OK\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                      "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                      "Content-Length: 0\r\n"
                      "\r\n";
            
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
        
        // Пропускаем первую строку (уже обработали)
        std::getline(iss, line);
        
        // Читаем заголовки
        while (std::getline(iss, line)) {
            if (line.empty() || line == "\r") break;
            
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            size_t colonPos = line.find(": ");
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 2);
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                headers[key] = value;
            }
        }
        
        // Читаем тело запроса
        std::string body;
        if (method == "POST" || method == "PUT") {
            std::string contentLengthStr = headers["content-length"];
            if (!contentLengthStr.empty()) {
                try {
                    size_t contentLength = std::stoul(contentLengthStr);
                    if (contentLength > 0 && contentLength < sizeof(buffer)) {
                        body.resize(contentLength);
                        
                        // Проверяем, что есть достаточно данных для чтения
                        auto currentPos = iss.tellg();
                        iss.seekg(0, std::ios::end);
                        auto endPos = iss.tellg();
                        iss.seekg(currentPos);
                        
                        size_t available = endPos - currentPos;
                        if (contentLength <= available) {
                            iss.read(&body[0], contentLength);
                        } else {
                            std::cout << "⚠️ Not enough data in stream. Expected: " 
                                      << contentLength << ", available: " << available << std::endl;
                            body.resize(available);
                            if (available > 0) {
                                iss.read(&body[0], available);
                            }
                        }
                    } else {
                        std::cout << "⚠️ Invalid content length: " << contentLength << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "❌ Error parsing content-length: " << e.what() << std::endl;
                }
            }
        }
        
        // Извлекаем токен сессии
        std::string sessionToken;
        auto authIt = headers.find("authorization");
        if (authIt != headers.end()) {
            std::string authHeader = authIt->second;
            if (authHeader.find("Bearer ") == 0) {
                sessionToken = authHeader.substr(7);
            } else {
                sessionToken = authHeader;
            }
            
            // Обрезаем пробелы
            sessionToken.erase(0, sessionToken.find_first_not_of(" \t\n\r\f\v"));
            sessionToken.erase(sessionToken.find_last_not_of(" \t\n\r\f\v") + 1);
        }
        
        // Обрабатываем запросы
        response = processRequest(method, path, body, sessionToken);
        
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleClient: " << e.what() << std::endl;
        response = createJsonResponse("{\"error\": \"Internal server error\"}", 500);
    }
    
    // Отправляем ответ
    if (!response.empty()) {
#ifdef _WIN32
        send(clientSocket, response.c_str(), response.length(), 0);
#else
        write(clientSocket, response.c_str(), response.length());
#endif
    }
    
    CLOSE_SOCKET(clientSocket);
}

std::string ApiService::processRequest(const std::string& method, const std::string& path, 
    const std::string& body, const std::string& sessionToken) {
    
    std::regex teacherRegex("^/teachers/(\\d+)$");
    std::regex studentRegex("^/students/(\\d+)$");
    std::regex groupRegex("^/groups/(\\d+)$");
    std::regex specializationRegex("^/specializations/(\\d+)$");
    std::regex teacherSpecializationsRegex("^/teachers/(\\d+)/specializations$");
    std::regex teacherSpecializationRegex("^/teachers/(\\d+)/specializations/(\\d+)$");
    std::smatch matches;

    try {
        std::cout << "🔄 Processing: " << method << " " << path << std::endl;

        if (method == "POST" && path == "/register") {
            return handleRegister(body);
        } else if (method == "POST" && path == "/login") {
            return handleLogin(body);
        } else if (method == "POST" && path == "/logout") {
            return handleLogout(sessionToken);
        } else if (method == "GET" && path == "/verify-token") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Invalid or expired token";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                std::string userId = getUserIdFromSession(sessionToken);
                json response;
                response["success"] = true;
                response["userId"] = userId;
                return createJsonResponse(response.dump());
            }
        } else if (method == "GET" && path == "/session-info") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return getSessionInfo(sessionToken);
            }
        } else if (method == "GET" && path == "/teachers") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return getTeachersJson(sessionToken);
            }
        } else if (method == "POST" && path == "/teachers") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return handleAddTeacher(body, sessionToken);
            }
        } else if (method == "PUT" && std::regex_match(path, matches, teacherRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int teacherId = std::stoi(matches[1]);
                return handleUpdateTeacher(body, teacherId, sessionToken);
            }
        } else if (method == "PUT" && path.find("/teachers/") == 0) {
            // Обработка случая с undefined в URL
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                try {
                    json j = json::parse(body);
                    if (j.contains("teacher_id") && !j["teacher_id"].is_null()) {
                        int teacherId = j["teacher_id"];
                        std::cout << "🔄 Extracted teacher_id from body: " << teacherId << std::endl;
                        return handleUpdateTeacher(body, teacherId, sessionToken);
                    } else {
                        json errorResponse;
                        errorResponse["success"] = false;
                        errorResponse["error"] = "Teacher ID is required";
                        return createJsonResponse(errorResponse.dump(), 400);
                    }
                } catch (const std::exception& e) {
                    json errorResponse;
                    errorResponse["success"] = false;
                    errorResponse["error"] = "Invalid request format";
                    return createJsonResponse(errorResponse.dump(), 400);
                }
            }
        } else if (method == "DELETE" && std::regex_match(path, matches, teacherRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int teacherId = std::stoi(matches[1]);
                return handleDeleteTeacher(teacherId, sessionToken);
            }
        } else if (method == "GET" && path == "/students") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return getStudentsJson(sessionToken);
            }
        } else if (method == "POST" && path == "/students") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return handleAddStudent(body, sessionToken);
            }
        } else if (method == "GET" && path == "/specializations") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return getSpecializationsJson(sessionToken);
            }
        } else if (method == "POST" && path == "/specializations") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return handleAddSpecialization(body, sessionToken);
            }
        } else if (method == "DELETE" && std::regex_match(path, matches, specializationRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int specializationCode = std::stoi(matches[1]);
                return handleDeleteSpecialization(specializationCode, sessionToken);
            }
        } else if (method == "PUT" && std::regex_match(path, matches, studentRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int studentId = std::stoi(matches[1]);
                return handleUpdateStudent(body, studentId, sessionToken);
            }
        } else if (method == "DELETE" && std::regex_match(path, matches, studentRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int studentId = std::stoi(matches[1]);
                return handleDeleteStudent(studentId, sessionToken);
            }
        } else if (method == "GET" && path == "/groups") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return getGroupsJson(sessionToken);
            }
        } else if (method == "POST" && path == "/groups") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return handleAddGroup(body, sessionToken);
            }
        } else if (method == "PUT" && std::regex_match(path, matches, groupRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int groupId = std::stoi(matches[1]);
                return handleUpdateGroup(body, groupId, sessionToken);
            }
        } else if (method == "DELETE" && std::regex_match(path, matches, groupRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int groupId = std::stoi(matches[1]);
                return handleDeleteGroup(groupId, sessionToken);
            }
        } else if (method == "GET" && path == "/profile") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return getProfile(sessionToken);
            }
        } else if (method == "PUT" && path == "/profile") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return handleUpdateProfile(body, sessionToken);
            }
        } else if (method == "POST" && std::regex_match(path, matches, teacherSpecializationsRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                return handleAddTeacherSpecialization(body, sessionToken);
            }
        } else if (method == "DELETE" && std::regex_match(path, matches, teacherSpecializationRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int teacherId = std::stoi(matches[1]);
                int specializationCode = std::stoi(matches[2]);
                return handleRemoveTeacherSpecialization(teacherId, specializationCode, sessionToken);
            }
        } else if (method == "GET" && std::regex_match(path, matches, teacherSpecializationsRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                int teacherId = std::stoi(matches[1]);
                return getTeacherSpecializationsJson(teacherId, sessionToken);
            }
        } else if (method == "GET" && path == "/api/status") {
            return handleStatus();
        } else {
            return createJsonResponse("{\"message\": \"Welcome to EduFlow API!\"}");
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in processRequest: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Internal server error";
        return createJsonResponse(errorResponse.dump(), 500);
    }
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
    
    // ПРОВЕРКА НА ПУСТОЙ КОНТЕНТ
    if (content.empty()) {
        std::cout << "⚠️ Warning: Empty content in createJsonResponse" << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 0\r\n"
               "\r\n";
    }
    
    std::stringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
             << "Content-Type: application/json\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
             << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
             << "Content-Length: " << content.length() << "\r\n"
             << "\r\n"
             << content;
    
    return response.str();
}

std::string ApiService::generateSessionToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    unsigned char buffer[32];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = static_cast<unsigned char>(dis(gen));
    }
    
    std::stringstream ss;
    for (size_t i = 0; i < sizeof(buffer); i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    return ss.str();
}

bool ApiService::validateSession(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sessionsMutex);
    
    auto it = sessions.find(token);
    if (it == sessions.end()) {
        return false;
    }
    
    // Проверяем, что данные сессии валидны
    if (it->second.userId.empty() || it->second.email.empty()) {
        sessions.erase(it);
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::hours>(now - it->second.lastActivity);
    
    if (duration.count() > apiConfig.sessionTimeoutHours) {
        sessions.erase(it);
        return false;
    }
    
    // Обновляем активность
    it->second.lastActivity = now;
    return true;
}

std::string ApiService::getUserIdFromSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(token);
    if (it != sessions.end()) {
        return it->second.userId;
    }
    
    return "";
}