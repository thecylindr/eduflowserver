#include "ApiService.h"
#include "json.hpp"
#include <sstream>
#include <cstring>

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
    char buffer[4096];
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
    
    // Simple HTTP request parsing
    if (request.find("GET /students") != std::string::npos) {
        response = getStudentsJson();
    } else if (request.find("GET /teachers") != std::string::npos) {
        response = getTeachersJson();
    } else if (request.find("GET /groups") != std::string::npos) {
        response = getGroupsJson();
    } else {
        response = createJsonResponse("{\"message\": \"Welcome to Student API (EduFlow)! Available endpoints: /students, /teachers, /groups\"}");
    }
    
#ifdef _WIN32
    send(clientSocket, response.c_str(), response.length(), 0);
#else
    write(clientSocket, response.c_str(), response.length());
#endif
    
    CLOSE_SOCKET(clientSocket);
}

std::string ApiService::createJsonResponse(const std::string& content) {
    std::stringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: application/json\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Content-Length: " << content.length() << "\r\n"
             << "\r\n"
             << content;
    return response.str();
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