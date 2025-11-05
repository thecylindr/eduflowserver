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
#include <cctype>

#ifndef _WIN32
    #include <fcntl.h>
    #include <errno.h>
    #include <arpa/inet.h>
#endif

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

using json = nlohmann::json;

// Глобальный объект для ограничения запросов
class RateLimiter {
private:
    std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> requests;
    std::mutex mutex;
    
public:
    bool isAllowed(const std::string& ip, size_t maxRequests = 100, std::chrono::seconds window = std::chrono::seconds(60)) {
        std::lock_guard<std::mutex> lock(mutex);
        auto now = std::chrono::steady_clock::now();
        
        // Удаляем старые запросы
        auto& timestamps = requests[ip];
        timestamps.erase(
            std::remove_if(timestamps.begin(), timestamps.end(),
                [now, window](const auto& timestamp) {
                    return now - timestamp > window;
                }),
            timestamps.end()
        );
        
        // Проверяем лимит
        if (timestamps.size() >= maxRequests) {
            return false;
        }
        
        timestamps.push_back(now);
        return true;
    }
};

static RateLimiter rateLimiter;

// Функция для логирования подозрительной активности
void logSuspiciousActivity(const std::string& request, const std::string& clientInfo) {
    std::ofstream logfile("security.log", std::ios_base::app);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    logfile << "[" << timeStr << "] SUSPICIOUS: " << clientInfo << " - " << request.substr(0, 200) << "\n";
    logfile.close();
}

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
        std::cout << "❌ Не удалось загрузить конфигурацию API" << std::endl;
        return false;
    }
    
    loadSessionsFromFile();
    
    // Создаем сокет
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET_VAL) {
        std::cout << "❌ Не удалось создать серверный сокет" << std::endl;
        return false;
    }
    
    int opt = 1;
    
    // КРОССПЛАТФОРМЕННАЯ НАСТРОЙКА СЕРВЕРНОГО СОКЕТА
#ifdef _WIN32
    // Windows: устанавливаем SO_REUSEADDR и неблокирующий режим
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cout << "⚠️ Не удалось установить SO_REUSEADDR" << std::endl;
    }
    
    // Увеличиваем буферы
    int recvBufSize = 65536;
    int sendBufSize = 65536;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufSize, sizeof(recvBufSize));
    setsockopt(serverSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize));
    
    // Серверный сокет в неблокирующий режим
    u_long mode = 1;
    if (ioctlsocket(serverSocket, FIONBIO, &mode) != 0) {
        std::cout << "❌ Не удалось установить неблокирующий режим для серверного сокета" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
#else
    // Unix/Linux
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cout << "❌ Не удалось установить параметры сокета" << std::endl;
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
    
    // Настраиваем адрес сервера
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    
    if (apiConfig.host == "0.0.0.0") {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        std::cout << "🌐 Сервер будет слушать на всех интерфейсах" << std::endl;
    } else {
        // Пробуем разные варианты для localhost
        if (apiConfig.host == "localhost") {
            serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
                serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            }
            std::cout << "🏠 Сервер будет слушать на localhost (127.0.0.1)" << std::endl;
        } else {
            serverAddr.sin_addr.s_addr = inet_addr(apiConfig.host.c_str());
            if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
                std::cout << "❌ Неверный адрес хоста: " << apiConfig.host << std::endl;
                CLOSE_SOCKET(serverSocket);
                return false;
            }
            std::cout << "🌐 Сервер будет слушать на " << apiConfig.host << std::endl;
        }
    }
    
    serverAddr.sin_port = htons(apiConfig.port);
    
    // Привязываем сокет
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cout << "❌ Не удалось привязать сокет к " << apiConfig.host << ":" << apiConfig.port << std::endl;
#ifdef _WIN32
        std::cout << "   Код ошибки Windows: " << WSAGetLastError() << std::endl;
#else
        std::cout << "   Код ошибки: " << errno << std::endl;
#endif
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    // Начинаем слушать
    if (listen(serverSocket, SOMAXCONN) < 0) {
        std::cout << "❌ Не удалось начать прослушивание на сокете" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    running = true;
    serverThread = std::thread(&ApiService::runServer, this);
    cleanupThread = std::thread(&ApiService::runCleanup, this);
    
    std::cout << "✅ API сервер успешно запущен!" << std::endl;
    std::cout << "📍 Адрес: " << apiConfig.host << ":" << apiConfig.port << std::endl;
    std::cout << "⏰ Таймаут сессии: " << apiConfig.sessionTimeoutHours << " часов" << std::endl;
    std::cout << "💻 Платформа: " << 
#ifdef _WIN32
        "Windows"
#else
        "Unix/Linux"
#endif
        << std::endl;
    
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
    std::cout << "🚀 Серверный поток запущен, ожидание подключений..." << std::endl;
    
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
            if (error == WSAEWOULDBLOCK) {
                // Нет ожидающих подключений - это нормально
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else if (error != WSAECONNRESET) {
                std::cout << "❌ Accept failed with error: " << error << std::endl;
            }
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else if (errno != ECONNABORTED) {
                std::cout << "❌ Accept failed with error: " << errno << std::endl;
            }
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // Получаем информацию о клиенте для логов
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        std::cout << "🔗 Новое подключение от " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        
        // Обрабатываем клиента в отдельном потоке
        std::thread clientThread(&ApiService::handleClient, this, clientSocket);
        clientThread.detach();
    }
    
    std::cout << "🛑 Серверный поток завершен" << std::endl;
}

void ApiService::handleClient(SOCKET_TYPE clientSocket) {
    // ПЕРЕКЛЮЧАЕМ КЛИЕНТСКИЙ СОКЕТ В БЛОКИРУЮЩИЙ РЕЖИМ ДЛЯ WINDOWS
#ifdef _WIN32
    u_long mode = 0; // 0 = блокирующий режим
    if (ioctlsocket(clientSocket, FIONBIO, &mode) != 0) {
        std::cout << "❌ Failed to set blocking mode for client socket" << std::endl;
        CLOSE_SOCKET(clientSocket);
        return;
    }
    
    // Устанавливаем разумные таймауты для Windows
    int timeout = 30000; // 30 секунд
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
#else
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif

    std::vector<char> buffer(8192); // Уменьшаем размер буфера
    std::string request;
    
    // Читаем данные порциями до получения полного HTTP запроса
    bool requestComplete = false;
    size_t totalBytesRead = 0;
    const size_t MAX_REQUEST_SIZE = 1024 * 1024; // 1MB максимум
    
    while (!requestComplete && totalBytesRead < MAX_REQUEST_SIZE) {
#ifdef _WIN32
        int bytesReceived = recv(clientSocket, buffer.data(), buffer.size() - 1, 0);
#else
        int bytesReceived = read(clientSocket, buffer.data(), buffer.size() - 1);
#endif
        
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            request.append(buffer.data(), bytesReceived);
            totalBytesRead += bytesReceived;
            
            // Проверяем, получен ли полный HTTP запрос
            size_t headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                // Если есть Content-Length, проверяем получено ли все тело
                size_t contentLengthHeader = request.find("Content-Length: ");
                if (contentLengthHeader != std::string::npos) {
                    size_t contentLengthEnd = request.find("\r\n", contentLengthHeader);
                    std::string contentLengthStr = request.substr(
                        contentLengthHeader + 16, contentLengthEnd - contentLengthHeader - 16);
                    
                    try {
                        size_t contentLength = std::stoul(contentLengthStr);
                        size_t bodyStart = headerEnd + 4;
                        if (request.length() >= bodyStart + contentLength) {
                            requestComplete = true;
                        }
                    } catch (...) {
                        // Если не можем распарсить Content-Length, считаем запрос полным
                        requestComplete = true;
                    }
                } else {
                    // Нет тела - запрос полный
                    requestComplete = true;
                }
            }
        } 
        else if (bytesReceived == 0) {
            // Соединение закрыто клиентом
            break;
        } 
        else {
            // Ошибка чтения
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT) {
                std::cout << "⏰ Таймаут при чтении от клиента" << std::endl;
            } else if (error != WSAECONNRESET) {
                std::cout << "❌ Ошибка recv: " << error << std::endl;
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != ECONNRESET) {
                std::cout << "❌ Ошибка read: " << errno << std::endl;
            }
#endif
            break;
        }
        
        // Небольшая пауза между чтениями
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    if (request.empty()) {
        CLOSE_SOCKET(clientSocket);
        return;
    }
    
    // Обрабатываем запрос
    std::string response = processRequestFromRaw(request);
    
    // Отправляем ответ
    if (!response.empty()) {
#ifdef _WIN32
        int totalSent = 0;
        const char* responseData = response.c_str();
        int responseLength = response.length();
        
        while (totalSent < responseLength) {
            int sent = send(clientSocket, responseData + totalSent, responseLength - totalSent, 0);
            if (sent == SOCKET_ERROR) {
                int error = WSAGetLastError();
                if (error == WSAETIMEDOUT) {
                    std::cout << "⏰ Таймаут при отправке клиенту" << std::endl;
                    break;
                } else if (error != WSAECONNRESET) {
                    std::cout << "❌ Ошибка send: " << error << std::endl;
                }
                break;
            }
            totalSent += sent;
        }
#else
        int totalSent = 0;
        const char* responseData = response.c_str();
        int responseLength = response.length();
        
        while (totalSent < responseLength) {
            int sent = write(clientSocket, responseData + totalSent, responseLength - totalSent);
            if (sent < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EPIPE && errno != ECONNRESET) {
                    std::cout << "❌ Ошибка write: " << errno << std::endl;
                }
                break;
            }
            totalSent += sent;
        }
#endif
    }
    
    // Корректно закрываем соединение
#ifdef _WIN32
    shutdown(clientSocket, SD_BOTH);
#else
    shutdown(clientSocket, SHUT_RDWR);
#endif
    CLOSE_SOCKET(clientSocket);
    
    std::cout << "✅ Обработан запрос, закрыто соединение" << std::endl;
}

// ф-ия обработки сырого запроса - ТОЛЬКО ОДНА РЕАЛИЗАЦИЯ
std::string ApiService::processRequestFromRaw(const std::string& rawRequest) {
    // ПРОВЕРКА НА МИНИМАЛЬНО ВАЛИДНЫЙ HTTP ЗАПРОС
    if (rawRequest.length() < 14) { // Минимум: "GET / HTTP/1.1"
        std::cout << "❌ Слишком короткий запрос: " << rawRequest.length() << " байт" << std::endl;
        logSuspiciousActivity(rawRequest, "Short request");
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid HTTP request\"}", 400);
    }
    
    // ПРОВЕРКА НА БАЗОВЫЙ HTTP СИНТАКСИС
    if (rawRequest.find("HTTP/") == std::string::npos) {
        std::cout << "❌ Не HTTP запрос" << std::endl;
        logSuspiciousActivity(rawRequest, "Not HTTP protocol");
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid HTTP protocol\"}", 400);
    }
    
    try {
        std::istringstream iss(rawRequest);
        std::string method, path, protocol;
        iss >> method >> path >> protocol;
        
        // ВАЛИДАЦИЯ МЕТОДА
        std::vector<std::string> allowedMethods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"};
        bool validMethod = false;
        for (const auto& m : allowedMethods) {
            if (method == m) {
                validMethod = true;
                break;
            }
        }
        
        if (!validMethod) {
            std::cout << "❌ Неподдерживаемый HTTP метод: " << method << std::endl;
            logSuspiciousActivity(rawRequest, "Invalid method: " + method);
            return createJsonResponse("{\"success\": false, \"error\": \"Method not allowed\"}", 405);
        }
        
        // ВАЛИДАЦИЯ ПУТИ
        if (path.empty() || path[0] != '/') {
            std::cout << "❌ Неверный путь: " << path << std::endl;
            logSuspiciousActivity(rawRequest, "Invalid path: " + path);
            return createJsonResponse("{\"success\": false, \"error\": \"Invalid path\"}", 400);
        }
        
        // ВАЛИДАЦИЯ ПРОТОКОЛА
        if (protocol != "HTTP/1.0" && protocol != "HTTP/1.1") {
            std::cout << "❌ Неподдерживаемый протокол: " << protocol << std::endl;
            logSuspiciousActivity(rawRequest, "Invalid protocol: " + protocol);
            return createJsonResponse("{\"success\": false, \"error\": \"Unsupported HTTP version\"}", 505);
        }
        
        std::cout << "✅ Валидный HTTP запрос: " << method << " " << path << " " << protocol << std::endl;
        
        // Извлекаем заголовки и тело
        std::unordered_map<std::string, std::string> headers;
        std::string line;
        std::string body;
        
        // Пропускаем первую строку
        std::getline(iss, line);
        
        // Читаем заголовки
        while (std::getline(iss, line)) {
            if (line.empty() || line == "\r") break;
            
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            // Пропускаем пустые строки
            if (line.empty()) continue;
            
            size_t colonPos = line.find(": ");
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 2);
                
                // Валидация ключа заголовка
                bool validHeader = true;
                for (char c : key) {
                    if (!std::isalnum(c) && c != '-') {
                        validHeader = false;
                        break;
                    }
                }
                
                if (validHeader) {
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    headers[key] = value;
                } else {
                    std::cout << "⚠️ Пропущен невалидный заголовок: " << key << std::endl;
                }
            }
        }
        
        // Читаем тело с ВАЛИДАЦИЕЙ
        if (method == "POST" || method == "PUT") {
            std::string contentLengthStr = headers["content-length"];
            if (!contentLengthStr.empty()) {
                try {
                    size_t contentLength = std::stoul(contentLengthStr);
                    
                    // ПРОВЕРКА РАЗМЕРА ТЕЛА
                    if (contentLength > 10 * 1024 * 1024) { // 10MB максимум
                        std::cout << "❌ Слишком большое тело запроса: " << contentLength << " байт" << std::endl;
                        return createJsonResponse("{\"success\": false, \"error\": \"Request body too large\"}", 413);
                    }
                    
                    if (contentLength > 0) {
                        body.resize(contentLength);
                        iss.read(&body[0], contentLength);
                        
                        // ПРОВЕРКА ЧТО ПРОЧИТАЛИ ВСЕ БАЙТЫ
                        size_t bytesRead = iss.gcount();
                        if (bytesRead != contentLength) {
                            std::cout << "❌ Несоответствие размера тела: ожидалось " << contentLength 
                                      << ", получено " << bytesRead << std::endl;
                            return createJsonResponse("{\"success\": false, \"error\": \"Incomplete request body\"}", 400);
                        }
                    }
                } catch (const std::exception& e) {
                    std::cout << "❌ Ошибка парсинга content-length: " << e.what() << std::endl;
                    return createJsonResponse("{\"success\": false, \"error\": \"Invalid Content-Length\"}", 400);
                }
            }
        }
        
        // Извлекаем токен с ВАЛИДАЦИЕЙ
        std::string sessionToken;
        auto authIt = headers.find("authorization");
        if (authIt != headers.end()) {
            std::string authHeader = authIt->second;
            
            // ВАЛИДАЦИЯ ФОРМАТА AUTHORIZATION HEADER
            if (authHeader.find("Bearer ") == 0) {
                sessionToken = authHeader.substr(7);
                // Проверяем что токен не пустой после Bearer
                if (sessionToken.empty()) {
                    std::cout << "⚠️ Пустой токен после Bearer" << std::endl;
                }
            } else {
                // Пробуем использовать как есть, но логируем
                sessionToken = authHeader;
                std::cout << "⚠️ Нестандартный Authorization header" << std::endl;
            }
            
            // ВАЛИДАЦИЯ ДЛИНЫ ТОКЕНА
            if (sessionToken.length() > 512) {
                std::cout << "❌ Слишком длинный токен: " << sessionToken.length() << " символов" << std::endl;
                return createJsonResponse("{\"success\": false, \"error\": \"Invalid token format\"}", 400);
            }
        }
        
        // ОБРАБАТЫВАЕМ ЗАПРОС
        std::string response = processRequest(method, path, body, sessionToken);
        
        // ПРОВЕРКА ЧТО PROCESSREQUEST ВЕРНУЛ ВАЛИДНЫЙ ОТВЕТ
        if (response.empty()) {
            std::cout << "❌ Пустой ответ от processRequest" << std::endl;
            return createJsonResponse("{\"success\": false, \"error\": \"Internal server error\"}", 500);
        }
        
        return response;
        
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в processRequestFromRaw: " << e.what() << std::endl;
        logSuspiciousActivity(rawRequest, "Exception: " + std::string(e.what()));
        return createJsonResponse("{\"success\": false, \"error\": \"Internal server error\"}", 500);
    }
}

std::string ApiService::processRequest(const std::string& method, const std::string& path, 
    const std::string& body, const std::string& sessionToken) {
    
    // 🔒 РАСШИРЕННАЯ ВАЛИДАЦИЯ ЗАПРОСА
    
    // Валидация метода
    if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE" && method != "OPTIONS") {
        std::cout << "🚨 Неподдерживаемый метод: " << method << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Method not allowed\"}", 405);
    }
    
    // Валидация длины пути
    if (path.length() > 1000) {
        std::cout << "🚨 Слишком длинный путь: " << path.length() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Path too long\"}", 414);
    }
    
    // Проверка на directory traversal и инъекции
    if (path.find("..") != std::string::npos || 
        path.find("//") != std::string::npos ||
        path.find("\\") != std::string::npos ||
        path.find("/./") != std::string::npos ||
        path.find("~") != std::string::npos ||
        path.find("%00") != std::string::npos) {
        std::cout << "🚨 Blocked path traversal attempt: " << path << std::endl;
        logSuspiciousActivity(path, "Path traversal attempt");
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid path\"}", 400);
    }
    
    // Проверка на бинарные данные в пути
    for (char c : path) {
        if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
            std::cout << "🚨 Blocked request with binary data in path" << std::endl;
            logSuspiciousActivity(path, "Binary data in path");
            return createJsonResponse("{\"success\": false, \"error\": \"Invalid characters in path\"}", 400);
        }
    }
    
    // Валидация тела запроса для POST/PUT
    if ((method == "POST" || method == "PUT") && !body.empty()) {
        try {
            // Пробуем распарсить JSON для валидации
            json j = json::parse(body);
        } catch (const std::exception& e) {
            std::cout << "❌ Невалидный JSON в теле запроса: " << e.what() << std::endl;
            return createJsonResponse("{\"success\": false, \"error\": \"Invalid JSON in request body\"}", 400);
        }
    }
    
    // 🔍 ОБЪЯВЛЕНИЕ РЕГУЛЯРНЫХ ВЫРАЖЕНИЙ ДЛЯ МАРШРУТИЗАЦИИ
    std::regex teacherRegex("^/teachers/(\\d+)$");
    std::regex studentRegex("^/students/(\\d+)$");
    std::regex groupRegex("^/groups/(\\d+)$");
    std::regex specializationRegex("^/specializations/(\\d+)$");
    std::regex teacherSpecializationsRegex("^/teachers/(\\d+)/specializations$");
    std::regex teacherSpecializationRegex("^/teachers/(\\d+)/specializations/(\\d+)$");
    std::smatch matches;

    try {
        std::cout << "🔄 Processing: " << method << " " << path << std::endl;

        // 🔐 АУТЕНТИФИКАЦИЯ И РЕГИСТРАЦИЯ
        if (method == "POST" && path == "/register") {
            return handleRegister(body);
        } else if (method == "POST" && path == "/login") {
            return handleLogin(body);
        } else if (method == "POST" && path == "/logout") {
            return handleLogout(sessionToken);
        } else if ((method == "GET" || method == "POST") && path == "/verify-token") {
            // ✅ Поддерживаем как GET, так и POST запросы для verify-token
            std::string tokenToValidate = sessionToken;
            
            // Если это POST запрос, пытаемся извлечь токен из тела
            if (method == "POST" && !body.empty()) {
                try {
                    json j = json::parse(body);
                    if (j.contains("token") && !j["token"].is_null()) {
                        tokenToValidate = j["token"];
                        std::cout << "🔐 Токен из тела запроса, длина: " << tokenToValidate.length() << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "⚠️ Не удалось распарсить тело verify-token запроса: " << e.what() << std::endl;
                    // Продолжаем с токеном из заголовка
                }
            }
            
            if (!validateSession(tokenToValidate)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Invalid or expired token";
                return createJsonResponse(errorResponse.dump(), 401);
            } else {
                std::string userId = getUserIdFromSession(tokenToValidate);
                json response;
                response["success"] = true;
                response["userId"] = userId;
                return createJsonResponse(response.dump());
            }
        
        // 👤 ПРОФИЛЬ И СЕССИИ
        } else if (method == "GET" && path == "/session-info") {
            return getSessionInfo(sessionToken);
        } else if (method == "GET" && path == "/profile") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getProfile(sessionToken);
        } else if (method == "PUT" && path == "/profile") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleUpdateProfile(body, sessionToken);
        } else if (method == "POST" && path == "/change-password") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleChangePassword(body, sessionToken);
        } else if (method == "GET" && path == "/sessions") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleGetSessions(sessionToken);
        } else if (method == "DELETE" && path == "/sessions") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleRevokeSession(body, sessionToken);
        
        // 👨‍🏫 УПРАВЛЕНИЕ ПРЕПОДАВАТЕЛЯМИ
        } else if (method == "GET" && path == "/teachers") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getTeachersJson(sessionToken);
        } else if (method == "POST" && path == "/teachers") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddTeacher(body, sessionToken);
        } else if (method == "PUT" && std::regex_match(path, matches, teacherRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int teacherId = std::stoi(matches[1]);
            return handleUpdateTeacher(body, teacherId, sessionToken);
        } else if (method == "PUT" && path.find("/teachers/") == 0) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
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
        } else if (method == "DELETE" && std::regex_match(path, matches, teacherRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int teacherId = std::stoi(matches[1]);
            return handleDeleteTeacher(teacherId, sessionToken);
        
        // 👨‍🎓 УПРАВЛЕНИЕ СТУДЕНТАМИ
        } else if (method == "GET" && path == "/students") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getStudentsJson(sessionToken);
        } else if (method == "POST" && path == "/students") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddStudent(body, sessionToken);
        } else if (method == "PUT" && std::regex_match(path, matches, studentRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int studentId = std::stoi(matches[1]);
            return handleUpdateStudent(body, studentId, sessionToken);
        } else if (method == "DELETE" && std::regex_match(path, matches, studentRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int studentId = std::stoi(matches[1]);
            return handleDeleteStudent(studentId, sessionToken);
        
        // 📚 УПРАВЛЕНИЕ СПЕЦИАЛИЗАЦИЯМИ
        } else if (method == "GET" && path == "/specializations") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getSpecializationsJson(sessionToken);
        } else if (method == "POST" && path == "/specializations") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddSpecialization(body, sessionToken);
        } else if (method == "DELETE" && std::regex_match(path, matches, specializationRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int specializationCode = std::stoi(matches[1]);
            return handleDeleteSpecialization(specializationCode, sessionToken);
        
        // 🔗 СПЕЦИАЛИЗАЦИИ ПРЕПОДАВАТЕЛЕЙ
        } else if (method == "POST" && std::regex_match(path, matches, teacherSpecializationsRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddTeacherSpecialization(body, sessionToken);
        } else if (method == "DELETE" && std::regex_match(path, matches, teacherSpecializationRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int teacherId = std::stoi(matches[1]);
            int specializationCode = std::stoi(matches[2]);
            return handleRemoveTeacherSpecialization(teacherId, specializationCode, sessionToken);
        } else if (method == "GET" && std::regex_match(path, matches, teacherSpecializationsRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int teacherId = std::stoi(matches[1]);
            return getTeacherSpecializationsJson(teacherId, sessionToken);
        
        // 👥 УПРАВЛЕНИЕ ГРУППАМИ
        } else if (method == "GET" && path == "/groups") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getGroupsJson(sessionToken);
        } else if (method == "POST" && path == "/groups") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddGroup(body, sessionToken);
        } else if (method == "PUT" && std::regex_match(path, matches, groupRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int groupId = std::stoi(matches[1]);
            return handleUpdateGroup(body, groupId, sessionToken);
        } else if (method == "DELETE" && std::regex_match(path, matches, groupRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int groupId = std::stoi(matches[1]);
            return handleDeleteGroup(groupId, sessionToken);
        
        // 📋 ПОРТФОЛИО И СОБЫТИЯ
        } else if (method == "GET" && path == "/portfolio") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getPortfolioJson(sessionToken);
        } else if (method == "POST" && path == "/portfolio") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddPortfolio(body, sessionToken);
        } else if (method == "GET" && path == "/events") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getEventsJson(sessionToken);
        } else if (method == "POST" && path == "/events") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddEvent(body, sessionToken);
        
        // ℹ️ СИСТЕМНАЯ ИНФОРМАЦИЯ
        } else if (method == "GET" && path == "/api/status") {
            return handleStatus();
        } else if (method == "GET" && path == "/") {
            // Welcome message for root path
            return createJsonResponse("{\"message\": \"Welcome to EduFlow API!\", \"version\": \"1.0\", \"status\": \"running\"}");
        
        // ❌ НЕИЗВЕСТНЫЙ ЭНДПОИНТ
        } else {
            // Для неизвестных путей возвращаем 404
            return createJsonResponse("{\"success\": false, \"error\": \"Endpoint not found\"}", 404);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in processRequest: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Internal server error";
        return createJsonResponse(errorResponse.dump(), 500);
    }
    
    // 🔒 ЗАЩИТА ОТ КОМПИЛЯТОРА - гарантированный возврат
    json errorResponse;
    errorResponse["success"] = false;
    errorResponse["error"] = "Unknown routing error";
    return createJsonResponse(errorResponse.dump(), 500);
}

std::string ApiService::createJsonResponse(const std::string& content, int statusCode) {
    // ВАЛИДАЦИЯ ВХОДНЫХ ДАННЫХ
    if (content.empty()) {
        std::cout << "⚠️ Пустой контент в createJsonResponse, статус: " << statusCode << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: 47\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
               "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
               "\r\n"
               "{\"success\":false,\"error\":\"Empty response\"}";
    }
    
    std::string statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 201: statusText = "Created"; break;
        case 400: statusText = "Bad Request"; break;
        case 401: statusText = "Unauthorized"; break;
        case 403: statusText = "Forbidden"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        case 413: statusText = "Payload Too Large"; break;
        case 500: statusText = "Internal Server Error"; break;
        case 505: statusText = "HTTP Version Not Supported"; break;
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

std::string ApiService::handleStatus() {
    json response;
    response["status"] = "running";
    response["version"] = "1.0";
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return createJsonResponse(response.dump());
}