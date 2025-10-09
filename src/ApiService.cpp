#include "ApiService.h"
#include "json.hpp"
#include <sstream>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/rand.h>

// Добавляем необходимые заголовки для Linux
#ifndef _WIN32
    #include <fcntl.h>
    #include <errno.h>
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
        std::cerr << "WSAStartup failed: " << result << std::endl;
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
    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET_VAL) {
#ifdef _WIN32
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
#else
        std::cerr << "Failed to create socket" << std::endl;
#endif
        return false;
    }
    
    // Set socket options for non-blocking behavior
    int opt = 1;
#ifdef _WIN32
    u_long mode = 1; // non-blocking
    ioctlsocket(serverSocket, FIONBIO, &mode);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    // Исправленный код для Linux
    int flags = fcntl(serverSocket, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl F_GETFL failed" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    if (fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl F_SETFL failed" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Setup server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(5000);
    
    // Bind socket
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    // Start listening
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    running = true;
    serverThread = std::thread(&ApiService::runServer, this);
    
    std::cout << "API server started on http://localhost:5000" << std::endl;
    return true;
}

void ApiService::stop() {
    if (!running) return;
    
    running = false;
    
    // Создаем временное соединение чтобы разблокировать accept()
    shutdownSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (shutdownSocket != INVALID_SOCKET_VAL) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serverAddr.sin_port = htons(5000);
        
        connect(shutdownSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        
        // Даем время на обработку
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        CLOSE_SOCKET(shutdownSocket);
        shutdownSocket = INVALID_SOCKET_VAL;
    }
    
    // Закрываем серверный сокет
    if (serverSocket != INVALID_SOCKET_VAL) {
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VAL;
    }
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    std::cout << "API server stopped" << std::endl;
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
        
        // Проверяем, не пора ли остановиться
        if (!running) break;
        
        if (clientSocket == INVALID_SOCKET_VAL) {
            // Для неблокирующих сокетов это нормально - продолжаем цикл
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cerr << "Accept failed: " << error << std::endl;
            }
#else
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            }
#endif
            // Короткая пауза чтобы не грузить CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Обрабатываем клиента в отдельном потоке или в этом же
        handleClient(clientSocket);
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
    
    // Parse HTTP method and path
    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;
    
    // Extract headers
    std::unordered_map<std::string, std::string> headers;
    std::string line;
    while (std::getline(iss, line) && line != "\r") {
        size_t pos = line.find(": ");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2);
            // Remove \r
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            headers[key] = value;
        }
    }
    
    // Extract body for POST requests
    std::string body;
    if (method == "POST" || method == "PUT") {
        size_t bodyPos = request.find("\r\n\r\n");
        if (bodyPos != std::string::npos) {
            body = request.substr(bodyPos + 4);
        }
    }
    
    // Extract session token from headers
    std::string sessionToken;
    if (headers.find("Authorization") != headers.end()) {
        std::string authHeader = headers["Authorization"];
        if (authHeader.find("Bearer ") == 0) {
            sessionToken = authHeader.substr(7);
        }
    }
    
    // Route requests
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
    } else if (method == "GET" && path.find("/students") != std::string::npos) {
        // Check authentication for protected endpoints
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getStudentsJson();
        }
    } else if (method == "GET" && path.find("/teachers") != std::string::npos) {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getTeachersJson();
        }
    } else if (method == "GET" && path.find("/groups") != std::string::npos) {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getGroupsJson();
        }
    } else if (method == "GET" && path == "/profile") {
        if (!validateSession(sessionToken)) {
            response = createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
        } else {
            response = getProfile(sessionToken);
        }
    } else {
        response = createJsonResponse("{\"message\": \"Welcome to Student API (EduFlow)! Available endpoints: /register, /login, /forgot-password, /reset-password, /logout, /profile, /students, /teachers, /groups\"}");
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
             << "Content-Type: application/json\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
             << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
             << "Content-Length: " << content.length() << "\r\n"
             << "\r\n"
             << content;
    return response.str();
}

std::string ApiService::hashPassword(const std::string& password) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if (!context) {
        return "";
    }

    if (EVP_DigestInit_ex(context, md, NULL) != 1 ||
        EVP_DigestUpdate(context, password.c_str(), password.length()) != 1 ||
        EVP_DigestFinal_ex(context, hash, &lengthOfHash) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }

    EVP_MD_CTX_free(context);

    std::stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
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
    
    // Check if session is expired (24 hours)
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::hours>(now - it->second.createdAt);
    if (duration.count() > 24) {
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

std::string ApiService::handleRegister(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string email = j["email"];
        std::string password = j["password"];
        std::string firstName = j["firstName"];
        std::string lastName = j["lastName"];
        std::string middleName = j.value("middleName", "");
        std::string phoneNumber = j.value("phoneNumber", "");
        
        // Check if user already exists
        if (dbService.getUserByEmail(email).userId != 0) {
            return createJsonResponse("{\"error\": \"User already exists\"}", 400);
        }
        
        // Create user
        User user;
        user.email = email;
        user.passwordHash = hashPassword(password);
        user.firstName = firstName;
        user.lastName = lastName;
        user.middleName = middleName;
        user.phoneNumber = phoneNumber;
        
        if (dbService.addUser(user)) {
            return createJsonResponse("{\"message\": \"User registered successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Registration failed\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleLogin(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string email = j["email"];
        std::string password = j["password"];
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0 || user.passwordHash != hashPassword(password)) {
            return createJsonResponse("{\"error\": \"Invalid email or password\"}", 401);
        }
        
        // Create session
        std::string sessionToken = generateSessionToken();
        Session session;
        session.userId = std::to_string(user.userId);
        session.email = user.email;
        session.createdAt = std::chrono::system_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions[sessionToken] = session;
        }
        
        json response;
        response["message"] = "Login successful";
        response["token"] = sessionToken;
        response["user"] = {
            {"userId", user.userId},
            {"email", user.email},
            {"firstName", user.firstName},
            {"lastName", user.lastName}
        };
        
        return createJsonResponse(response.dump());
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleForgotPassword(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string email = j["email"];
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            // Don't reveal whether user exists
            return createJsonResponse("{\"message\": \"If the email exists, a reset code has been sent\"}");
        }
        
        // Generate reset token (in a real app, send this via email)
        std::string resetToken = generateSessionToken();
        
        {
            std::lock_guard<std::mutex> lock(passwordResetMutex);
            passwordResetTokens[resetToken] = PasswordResetToken{
                email,
                std::chrono::system_clock::now()
            };
        }
        
        // In a real application, you would send an email here
        // For demo purposes, we'll return the token
        json response;
        response["message"] = "Reset code generated";
        response["resetToken"] = resetToken; // Remove this in production!
        
        return createJsonResponse(response.dump());
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleResetPassword(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string resetToken = j["resetToken"];
        std::string newPassword = j["newPassword"];
        
        std::string email;
        {
            std::lock_guard<std::mutex> lock(passwordResetMutex);
            auto it = passwordResetTokens.find(resetToken);
            if (it == passwordResetTokens.end()) {
                return createJsonResponse("{\"error\": \"Invalid or expired reset token\"}", 400);
            }
            
            // Check if token is expired (1 hour)
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.createdAt);
            if (duration.count() > 60) {
                passwordResetTokens.erase(it);
                return createJsonResponse("{\"error\": \"Reset token expired\"}", 400);
            }
            
            email = it->second.email;
            passwordResetTokens.erase(it);
        }
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            return createJsonResponse("{\"error\": \"User not found\"}", 404);
        }
        
        // Update password
        user.passwordHash = hashPassword(newPassword);
        if (dbService.updateUser(user)) {
            return createJsonResponse("{\"message\": \"Password reset successfully\"}");
        } else {
            return createJsonResponse("{\"error\": \"Password reset failed\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleLogout(const std::string& sessionToken) {
    if (!sessionToken.empty()) {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        sessions.erase(sessionToken);
    }
    
    return createJsonResponse("{\"message\": \"Logged out successfully\"}");
}

std::string ApiService::getProfile(const std::string& sessionToken) {
    std::string userId = getUserIdFromSession(sessionToken);
    if (userId.empty()) {
        return createJsonResponse("{\"error\": \"Invalid session\"}", 401);
    }
    
    User user = dbService.getUserById(std::stoi(userId));
    if (user.userId == 0) {
        return createJsonResponse("{\"error\": \"User not found\"}", 404);
    }
    
    json userJson;
    userJson["userId"] = user.userId;
    userJson["email"] = user.email;
    userJson["firstName"] = user.firstName;
    userJson["lastName"] = user.lastName;
    userJson["middleName"] = user.middleName;
    userJson["phoneNumber"] = user.phoneNumber;
    
    return createJsonResponse(userJson.dump());
}

std::string ApiService::getStudentsJson() {
    auto students = dbService.getStudents();
    json j = json::array();
    
    for (const auto& student : students) {
        json studentJson;
        studentJson["studentCode"] = student.studentCode;
        studentJson["lastName"] = student.lastName;
        studentJson["firstName"] = student.firstName;
        studentJson["middleName"] = student.middleName;
        studentJson["phoneNumber"] = student.phoneNumber;
        studentJson["email"] = student.email;
        studentJson["groupId"] = student.groupId;
        
        j.push_back(studentJson);
    }
    
    return createJsonResponse(j.dump(4));
}

std::string ApiService::getTeachersJson() {
    auto teachers = dbService.getTeachers();
    json j = json::array();
    
    for (const auto& teacher : teachers) {
        json teacherJson;
        teacherJson["teacherId"] = teacher.teacherId;
        teacherJson["lastName"] = teacher.lastName;
        teacherJson["firstName"] = teacher.firstName;
        teacherJson["middleName"] = teacher.middleName;
        teacherJson["experience"] = teacher.experience;
        teacherJson["specialization"] = teacher.specialization;
        teacherJson["email"] = teacher.email;
        teacherJson["phoneNumber"] = teacher.phoneNumber;
        
        j.push_back(teacherJson);
    }
    
    return createJsonResponse(j.dump(4));
}

std::string ApiService::getGroupsJson() {
    auto groups = dbService.getGroups();
    json j = json::array();
    
    for (const auto& group : groups) {
        json groupJson;
        groupJson["groupId"] = group.groupId;
        groupJson["name"] = group.name;
        groupJson["studentCount"] = group.studentCount;
        groupJson["teacherId"] = group.teacherId;
        
        j.push_back(groupJson);
    }
    
    return createJsonResponse(j.dump(4));
}