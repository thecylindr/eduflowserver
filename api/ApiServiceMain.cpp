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
    loadSessionsFromDB();
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
        if (apiConfig.host == "localhost" || apiConfig.host == "127.0.0.1") {
            serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        } else {
            serverAddr.sin_addr.s_addr = inet_addr(apiConfig.host.c_str());
        }
    }
    
    serverAddr.sin_port = htons(apiConfig.port);
    
    // Биндим сокет
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cout << "❌ Не удалось забиндить сокет на " << apiConfig.host << ":" << apiConfig.port << std::endl;
#ifdef _WIN32
        std::cout << "Ошибка: " << WSAGetLastError() << std::endl;
#else
        std::cout << "Ошибка: " << strerror(errno) << std::endl;
#endif
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    // Слушаем
    if (listen(serverSocket, SOMAXCONN) < 0) {
        std::cout << "❌ Не удалось начать прослушивание" << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    running = true;
    serverThread = std::thread(&ApiService::runServer, this);
    cleanupThread = std::thread(&ApiService::runCleanup, this);
    
    std::cout << "🚀 Сервер запущен на " << apiConfig.host << ":" << apiConfig.port << std::endl;
    return true;
}

void ApiService::stop() {
    if (!running) return;
    
    std::cout << "🛑 Останавливаем API сервер..." << std::endl;
    running = false;
    
    // Закрываем серверный сокет чтобы прервать accept
    if (serverSocket != INVALID_SOCKET_VAL) {
        // Создаем временное соединение чтобы разблокировать accept
        SOCKET_TYPE tempSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (tempSocket != INVALID_SOCKET_VAL) {
            sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            serverAddr.sin_port = htons(apiConfig.port);
            
            // Пытаемся подключиться чтобы разблокировать accept
            connect(tempSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
            
            // Даем время на обработку
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            CLOSE_SOCKET(tempSocket);
        }
        
        // Теперь закрываем серверный сокет
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET_VAL;
    }
    
    // Ждем завершения потоков
    if (serverThread.joinable()) {
        std::cout << "⏳ Ждем завершения серверного потока..." << std::endl;
        serverThread.join();
        std::cout << "✅ Серверный поток завершен" << std::endl;
    }
    
    if (cleanupThread.joinable()) {
        std::cout << "⏳ Ждем завершения потока очистки..." << std::endl;
        cleanupThread.join();
        std::cout << "✅ Поток очистки завершен" << std::endl;
    }
    
    std::cout << "✅ API сервер полностью остановлен" << std::endl;
}

void ApiService::runServer() {
    std::cout << "🚀 Серверный поток запущен" << std::endl;
    
    while (running) {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        
        // Используем select для неблокирующего accept с таймаутом
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;  // Таймаут 1 секунда
        timeout.tv_usec = 0;
        
        int activity = select(serverSocket + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity < 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err != WSAEINTR) {
                std::cout << "❌ Ошибка select: " << err << std::endl;
            }
#else
            if (errno != EINTR) {
                std::cout << "❌ Ошибка select: " << strerror(errno) << std::endl;
            }
#endif
            continue;
        }
        
        // Проверяем, был ли остановлен сервер во время ожидания
        if (!running) break;
        
        if (activity == 0) {
            // Таймаут - продолжаем цикл
            continue;
        }
        
        if (FD_ISSET(serverSocket, &readfds)) {
            SOCKET_TYPE clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
            
            if (clientSocket == INVALID_SOCKET_VAL) {
#ifdef _WIN32
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) {
                    continue;
                }
                if (err != WSAEINTR) {
                    std::cout << "❌ Ошибка accept: " << err << std::endl;
                }
#else
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    continue;
                }
                if (errno != EINTR) {
                    std::cout << "❌ Ошибка accept: " << strerror(errno) << std::endl;
                }
#endif
                continue;
            }
            
            // Настраиваем клиентский сокет
#ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(clientSocket, FIONBIO, &mode);
#else
            int flags = fcntl(clientSocket, F_GETFL, 0);
            fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
#endif
            
            handleClient(clientSocket);
        }
    }
    
    std::cout << "🔴 Серверный поток завершает работу" << std::endl;
}


std::string ApiService::getClientInfo(SOCKET_TYPE clientSocket) {
#ifdef _WIN32
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    if (getpeername(clientSocket, (sockaddr*)&clientAddr, &addrLen) == 0) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, INET_ADDRSTRLEN);
        return std::string(ip);
    } else {
        int error = WSAGetLastError();
        std::cout << "❌ Ошибка получения IP клиента: " << error << std::endl;
        return "unknown";
    }
#else
    sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    if (getpeername(clientSocket, (sockaddr*)&clientAddr, &addrLen) == 0) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, INET_ADDRSTRLEN);
        return std::string(ip);
    } else {
        std::cout << "❌ Ошибка получения IP клиента: " << strerror(errno) << std::endl;
        return "unknown";
    }
#endif
}

void ApiService::handleClient(SOCKET_TYPE clientSocket) {
    std::string clientIP = getClientInfo(clientSocket);
    std::cout << "🔗 Новое подключение от IP: " << clientIP << std::endl;
    
    std::string rawRequest;
    char buffer[4096];
    int bytesReceived;
    auto startTime = std::chrono::steady_clock::now();

    // Увеличиваем таймаут и улучшаем чтение
    while (true) {
        memset(buffer, 0, sizeof(buffer));  // Очищаем буфер
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);  // Оставляем место для нуль-терминатора
        
        if (bytesReceived > 0) {
            rawRequest.append(buffer, bytesReceived);
            
            // Проверяем конец заголовков
            if (rawRequest.find("\r\n\r\n") != std::string::npos) {
                // Если есть Content-Length, читаем тело
                size_t headersEnd = rawRequest.find("\r\n\r\n");
                std::string headers = rawRequest.substr(0, headersEnd);
                
                // Ищем Content-Length
                size_t clPos = headers.find("Content-Length:");
                if (clPos != std::string::npos) {
                    size_t clEnd = headers.find("\r\n", clPos);
                    std::string clStr = headers.substr(clPos + 15, clEnd - clPos - 15);
                    try {
                        size_t contentLength = std::stoul(clStr);
                        size_t bodyStart = headersEnd + 4;
                        if (rawRequest.length() - bodyStart >= contentLength) {
                            break;  // Все данные получены
                        }
                    } catch (...) {
                        // Если не удалось распарсить Content-Length, считаем что заголовки закончились
                        break;
                    }
                } else {
                    // Нет тела - запрос завершен
                    break;
                }
            }
        } else if (bytesReceived == 0) {
            std::cout << "🔌 Клиент отключился: " << clientIP << std::endl;
            break;
        } else {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
#else
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
#endif
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count() > 30) {  // Увеличили таймаут
                    std::cout << "⏰ Таймаут чтения от клиента: " << clientIP << std::endl;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Увеличили задержку
                continue;
            }
            std::cout << "❌ Ошибка чтения от клиента " << clientIP << ": ";
#ifdef _WIN32
            std::cout << WSAGetLastError();
#else
            std::cout << strerror(errno);
#endif
            std::cout << std::endl;
            break;
        }
    }

    if (rawRequest.empty()) {
        std::cout << "📭 Пустой запрос от клиента: " << clientIP << std::endl;
        CLOSE_SOCKET(clientSocket);
        return;
    }

    std::cout << "📨 Получен запрос от " << clientIP << ", размер: " << rawRequest.length() << " байт" << std::endl;
    
    // Обрабатываем запрос
    std::string response = processRequestFromRaw(rawRequest, clientIP);
    
    // Отправляем ответ
    int totalSent = 0;
    const char* responseData = response.c_str();
    size_t responseLength = response.length();
    
    while (totalSent < static_cast<int>(responseLength)) {
        int bytesSent = send(clientSocket, responseData + totalSent, responseLength - totalSent, 0);
        if (bytesSent <= 0) {
            std::cout << "❌ Ошибка отправки ответа клиенту " << clientIP << std::endl;
            break;
        }
        totalSent += bytesSent;
    }
    
    if (totalSent > 0) {
        std::cout << "📤 Ответ отправлен клиенту " << clientIP << ", размер: " << totalSent << " байт" << std::endl;
    }
    
    CLOSE_SOCKET(clientSocket);
    std::cout << "🔌 Соединение с клиентом " << clientIP << " закрыто" << std::endl;
}

void ApiService::runCleanup() {
    std::cout << "🧹 Поток очистки запущен" << std::endl;
    
    while (running) {
        cleanupExpiredSessions();
        dbService.deleteExpiredSessions();
        
        // Используем прерываемый sleep
        for (int i = 0; i < 300 && running; i++) { // 5 минут = 300 секунд
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cout << "🔴 Поток очистки завершает работу" << std::endl;
}

std::string ApiService::processRequestFromRaw(const std::string& rawRequest, const std::string& clientIP) {
    // ПРОВЕРКА НА МИНИМАЛЬНО ВАЛИДНЫЙ HTTP ЗАПРОС
    if (rawRequest.length() < 14) {
        std::cout << "❌ Слишком короткий запрос от " << clientIP << ": " << rawRequest.length() << " байт" << std::endl;
        logSuspiciousActivity(rawRequest, "Short request from IP: " + clientIP);
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid HTTP request\"}", 400);
    }
    
    // ПРОВЕРКА НА БАЗОВЫЙ HTTP СИНТАКСИС
    if (rawRequest.find("HTTP/") == std::string::npos) {
        std::cout << "❌ Не HTTP запрос от " << clientIP << std::endl;
        logSuspiciousActivity(rawRequest, "Not HTTP protocol from IP: " + clientIP);
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
            std::cout << "❌ Неподдерживаемый HTTP метод от " << clientIP << ": " << method << std::endl;
            logSuspiciousActivity(rawRequest, "Invalid method from IP: " + clientIP + " - " + method);
            return createJsonResponse("{\"success\": false, \"error\": \"Method not allowed\"}", 405);
        }
        
        // ВАЛИДАЦИЯ ПУТИ
        if (path.empty() || path[0] != '/') {
            std::cout << "❌ Неверный путь от " << clientIP << ": " << path << std::endl;
            logSuspiciousActivity(rawRequest, "Invalid path from IP: " + clientIP + " - " + path);
            return createJsonResponse("{\"success\": false, \"error\": \"Invalid path\"}", 400);
        }
        
        // ВАЛИДАЦИЯ ПРОТОКОЛА
        if (protocol != "HTTP/1.0" && protocol != "HTTP/1.1") {
            std::cout << "❌ Неподдерживаемый протокол от " << clientIP << ": " << protocol << std::endl;
            logSuspiciousActivity(rawRequest, "Invalid protocol from IP: " + clientIP + " - " + protocol);
            return createJsonResponse("{\"success\": false, \"error\": \"Unsupported HTTP version\"}", 505);
        }
        
        std::cout << "✅ Валидный HTTP запрос от " << clientIP << ": " << method << " " << path << " " << protocol << std::endl;
        
        // Извлекаем заголовки и тело
        std::unordered_map<std::string, std::string> headers;
        std::string line;
        std::string body;
        
        // Пропускаем первую строку
        std::getline(iss, line);
        
        // Читаем заголовки
        std::string userOS = "unknown";
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
                    
                    // Сохраняем OS
                    if (key == "user-os") {
                        userOS = value;
                    }
                } else {
                    std::cout << "⚠️ Пропущен невалидный заголовок от " << clientIP << ": " << key << std::endl;
                }
            }
        }
        
        // Читаем тело для POST, PUT и DELETE запросов
        if (method == "POST" || method == "PUT" || method == "DELETE") {
            std::string contentLengthStr = headers["content-length"];
            if (!contentLengthStr.empty()) {
                try {
                    size_t contentLength = std::stoul(contentLengthStr);
                    
                    if (contentLength > 10 * 1024 * 1024) {
                        std::cout << "❌ Слишком большое тело запроса от " << clientIP << ": " << contentLength << " байт" << std::endl;
                        return createJsonResponse("{\"success\": false, \"error\": \"Request body too large\"}", 413);
                    }
                    
                    if (contentLength > 0) {
                        body.resize(contentLength);
                        iss.read(&body[0], contentLength);
                        
                        size_t bytesRead = iss.gcount();
                        if (bytesRead != contentLength) {
                            std::cout << "❌ Несоответствие размера тела от " << clientIP 
                                      << ": ожидалось " << contentLength << ", получено " << bytesRead << std::endl;
                            return createJsonResponse("{\"success\": false, \"error\": \"Incomplete request body\"}", 400);
                        }
                        
                        std::cout << "📦 Тело запроса прочитано: " << body.length() << " байт" << std::endl;
                        std::cout << "📦 Содержимое тела: " << body << std::endl;
                    } else {
                        std::cout << "⚠️ Content-Length = 0 для " << method << " запроса" << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cout << "❌ Ошибка парсинга content-length от " << clientIP << ": " << e.what() << std::endl;
                    return createJsonResponse("{\"success\": false, \"error\": \"Invalid Content-Length\"}", 400);
                }
            } else {
                std::cout << "❌ " << method << " запрос без Content-Length, но ожидается тело" << std::endl;
                // Для DELETE запросов с телом Content-Length обязателен
                if (!body.empty()) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Content-Length header required\"}", 411);
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
                if (sessionToken.empty()) {
                    std::cout << "⚠️ Пустой токен после Bearer от " << clientIP << std::endl;
                } else {
                    std::cout << "🔐 Токен получен от " << clientIP << ", длина: " << sessionToken.length() << std::endl;
                }
            } else {
                sessionToken = authHeader;
                std::cout << "⚠️ Нестандартный Authorization header от " << clientIP << std::endl;
            }
            
            // ВАЛИДАЦИЯ ДЛИНЫ ТОКЕНА
            if (sessionToken.length() > 512) {
                std::cout << "❌ Слишком длинный токен от " << clientIP << ": " << sessionToken.length() << " символов" << std::endl;
                return createJsonResponse("{\"success\": false, \"error\": \"Invalid token format\"}", 400);
            }
        }
        
        // Формируем clientInfo для передачи в processRequest
        std::string clientInfo = "IP: " + clientIP + ", OS: " + userOS;
        std::cout << "👤 ClientInfo: " << clientInfo << std::endl;
        
        // ОБРАБАТЫВАЕМ ЗАПРОС с передачей clientInfo
        std::string response = processRequest(method, path, body, sessionToken, clientInfo);
        
        // ПРОВЕРКА ЧТО PROCESSREQUEST ВЕРНУЛ ВАЛИДНЫЙ ОТВЕТ
        if (response.empty()) {
            std::cout << "❌ Пустой ответ от processRequest для клиента " << clientIP << std::endl;
            return createJsonResponse("{\"success\": false, \"error\": \"Internal server error\"}", 500);
        }
        
        return response;
        
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в processRequestFromRaw для клиента " << clientIP << ": " << e.what() << std::endl;
        logSuspiciousActivity(rawRequest, "Exception from IP: " + clientIP + " - " + std::string(e.what()));
        return createJsonResponse("{\"success\": false, \"error\": \"Internal server error\"}", 500);
    }
}

std::string ApiService::processRequest(const std::string& method, const std::string& path, 
    const std::string& body, const std::string& sessionToken, const std::string& clientInfo) {
    
    // 🔒 РАСШИРЕННАЯ ВАЛИДАЦИЯ ЗАПРОСА
    
    // Извлекаем IP и User-OS из clientInfo
    std::string clientIP = "unknown";
    std::string userOS = "unknown";
    
    size_t ipPos = clientInfo.find("IP: ");
    size_t uaPos = clientInfo.find("OS: ");
    
    if (ipPos != std::string::npos) {
        size_t ipEnd = clientInfo.find(",", ipPos);
        if (ipEnd != std::string::npos) {
            clientIP = clientInfo.substr(ipPos + 4, ipEnd - ipPos - 4);
        } else {
            // Если запятая не найдена, берем до конца строки
            clientIP = clientInfo.substr(ipPos + 4);
        }
    }
    if (uaPos != std::string::npos) {
        // Исправляем смещение - "OS: " имеет длину 4 символа
        userOS = clientInfo.substr(uaPos + 4);
    }
    
    std::cout << "🔍 Обработка запроса от " << clientIP << " (" << userOS << ")" << std::endl;
    
    // Валидация метода
    if (method != "GET" && method != "POST" && method != "PUT" && method != "DELETE" && method != "OPTIONS") {
        std::cout << "🚨 Неподдерживаемый метод от " << clientIP << ": " << method << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Method not allowed\"}", 405);
    }
    
    // Валидация длины пути
    if (path.length() > 1000) {
        std::cout << "🚨 Слишком длинный путь от " << clientIP << ": " << path.length() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Path too long\"}", 414);
    }
    
    // Проверка на directory traversal и инъекции
    if (path.find("..") != std::string::npos || 
        path.find("//") != std::string::npos ||
        path.find("\\") != std::string::npos ||
        path.find("/./") != std::string::npos ||
        path.find("~") != std::string::npos ||
        path.find("%00") != std::string::npos) {
        std::cout << "🚨 Blocked path traversal attempt от " << clientIP << ": " << path << std::endl;
        logSuspiciousActivity(path, "Path traversal attempt from IP: " + clientIP);
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid path\"}", 400);
    }
    
    // Проверка на бинарные данные в пути
    for (char c : path) {
        if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
            std::cout << "🚨 Blocked request with binary data in path от " << clientIP << std::endl;
            logSuspiciousActivity(path, "Binary data in path from IP: " + clientIP);
            return createJsonResponse("{\"success\": false, \"error\": \"Invalid characters in path\"}", 400);
        }
    }
    
    // Валидация тела запроса для POST/PUT
    if ((method == "POST" || method == "PUT") && !body.empty()) {
        try {
            // Пробуем распарсить JSON для валидации
            json j = json::parse(body);
        } catch (const std::exception& e) {
            std::cout << "❌ Невалидный JSON в теле запроса от " << clientIP << ": " << e.what() << std::endl;
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
    std::regex sessionTokenRegex("^/sessions/([a-fA-F0-9]+)$");
    std::regex eventRegex("^/events/(\\d+)$");
    std::regex portfolioRegex("^/portfolio/(\\d+)$");
    std::regex eventCategoryRegex("^/event-categories/(\\d+)$");
    std::regex groupStudentsRegex("^/groups/(\\d+)/students$");
    std::smatch matches;
    
    try {
        std::cout << "🔄 Processing от " << clientIP << ": " << method << " " << path << std::endl;
        
        // 🔐 АУТЕНТИФИКАЦИЯ И РЕГИСТРАЦИЯ
        if (method == "POST" && path == "/register") {
            return handleRegister(body, clientInfo);
        } else if (method == "POST" && path == "/login") {
            return handleLogin(body, clientInfo);
        } else if (method == "POST" && path == "/logout") {
            return handleLogout(sessionToken, clientInfo);
        } else if ((method == "GET" || method == "POST") && path == "/verify-token") {
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
                }
            }
            
            if (tokenToValidate.empty()) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["valid"] = false;
                errorResponse["error"] = "Token is required";
                return createJsonResponse(errorResponse.dump(), 400);
            }
            
            // УНИВЕРСАЛЬНАЯ ПРОВЕРКА ТОКЕНА
            bool isValid = validateTokenInDatabase(tokenToValidate);
            
            if (isValid) {
                // Токен валиден, получаем информацию о пользователе
                std::string userId = getUserIdFromSession(tokenToValidate);
                User user = dbService.getUserById(std::stoi(userId));
                
                if (user.userId == 0) {
                    json errorResponse;
                    errorResponse["success"] = false;
                    errorResponse["valid"] = false;
                    errorResponse["error"] = "User not found";
                    return createJsonResponse(errorResponse.dump(), 404);
                }
                
                // ФОРМИРУЕМ УНИВЕРСАЛЬНЫЙ ОТВЕТ ДЛЯ КЛИЕНТА
                json responseData;
                responseData["valid"] = true;
                responseData["userId"] = userId;
                responseData["user"] = {
                    {"userId", user.userId},
                    {"login", user.login},
                    {"email", user.email},
                    {"firstName", user.firstName},
                    {"lastName", user.lastName},
                    {"middleName", user.middleName},
                    {"phoneNumber", user.phoneNumber}
                };
                
                json response;
                response["success"] = true;
                response["valid"] = true;
                response["data"] = responseData;
                response["message"] = "Token is valid";
                
                std::cout << "✅ Token validated successfully for user: " << user.login << std::endl;
                return createJsonResponse(response.dump());
            } else {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["valid"] = false;
                errorResponse["error"] = "Invalid or expired token";
                return createJsonResponse(errorResponse.dump(), 401);
            }
        
        } else if (method == "GET" && path == "/dashboard") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleGetDashboard(sessionToken);
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
        
        // НОВЫЙ МАРШРУТ ДЛЯ УДАЛЕНИЯ СЕССИИ ПО ТОКЕНУ В URL
        } else if (method == "DELETE" && std::regex_match(path, matches, sessionTokenRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            std::string targetToken = matches[1];
            return handleRevokeSessionByToken(targetToken, sessionToken);
        
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
            return handleAddTeacher(body);
        } else if (method == "PUT" && std::regex_match(path, matches, teacherRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int teacherId = std::stoi(matches[1]);
            return handleUpdateTeacher(body, teacherId);
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
                    std::cout << "🔄 Extracted teacher_id from body от " << clientIP << ": " << teacherId << std::endl;
                    return handleUpdateTeacher(body, teacherId);
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
            return handleDeleteTeacher(teacherId);
        
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
            return handleAddStudent(body);
        } else if (method == "PUT" && std::regex_match(path, matches, studentRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int studentId = std::stoi(matches[1]);
            return handleUpdateStudent(body, studentId);
        } else if (method == "DELETE" && std::regex_match(path, matches, studentRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int studentId = std::stoi(matches[1]);
            return handleDeleteStudent(studentId);
        
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
            return handleAddSpecialization(body);
        } else if (method == "DELETE" && std::regex_match(path, matches, specializationRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int specializationCode = std::stoi(matches[1]);
            return handleDeleteSpecialization(specializationCode);
        
        // 🔗 СПЕЦИАЛИЗАЦИИ ПРЕПОДАВАТЕЛЕЙ
        } else if (method == "POST" && std::regex_match(path, matches, teacherSpecializationsRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddTeacherSpecialization(body);
        } else if (method == "DELETE" && std::regex_match(path, matches, teacherSpecializationRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int teacherId = std::stoi(matches[1]);
            int specializationCode = std::stoi(matches[2]);
            return handleRemoveTeacherSpecialization(teacherId, specializationCode);
        } else if (method == "GET" && std::regex_match(path, matches, teacherSpecializationsRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int teacherId = std::stoi(matches[1]);
            return getTeacherSpecializationsJson(teacherId);
        
        // 👥 УПРАВЛЕНИЕ ГРУППАМИ
        } else if (method == "GET" && path == "/groups") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getGroupsJson(sessionToken);
        } else if (method == "GET" && std::regex_match(path, matches, groupStudentsRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int groupId = std::stoi(matches[1]);
            return handleGetStudentsByGroup(groupId);
        } else if (method == "POST" && path == "/groups") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddGroup(body);
        } else if (method == "PUT" && std::regex_match(path, matches, groupRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int groupId = std::stoi(matches[1]);
            return handleUpdateGroup(body, groupId);
        } else if (method == "DELETE" && std::regex_match(path, matches, groupRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int groupId = std::stoi(matches[1]);
            return handleDeleteGroup(groupId);
        
        // 📋 ПОРТФОЛИО - полный CRUD
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
            return handleAddPortfolio(body);
        } else if (method == "PUT" && std::regex_match(path, matches, portfolioRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int portfolioId = std::stoi(matches[1]);
            return handleUpdatePortfolio(body, portfolioId);
        } else if (method == "DELETE" && std::regex_match(path, matches, portfolioRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int portfolioId = std::stoi(matches[1]);
            return handleDeletePortfolio(portfolioId);
        
        // 📅 СОБЫТИЯ - полный CRUD
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
            return handleAddEvent(body);
        } else if (method == "PUT" && std::regex_match(path, matches, eventRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int eventId = std::stoi(matches[1]);
            return handleUpdateEvent(body, eventId);
        } else if (method == "DELETE" && std::regex_match(path, matches, eventRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int eventId = std::stoi(matches[1]);
            return handleDeleteEvent(eventId);
        } else if (method == "GET" && path == "/event-categories") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return getEventCategoriesJson();
        }
        else if (method == "POST" && path == "/event-categories") {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            return handleAddEventCategory(body);
        }
        else if (method == "PUT" && std::regex_match(path, matches, eventCategoryRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int eventCode = std::stoi(matches[1]);  // ИЗВЛЕКАЕМ ЧИСЛО
            return handleUpdateEventCategory(body, eventCode);
        }
        else if (method == "DELETE" && std::regex_match(path, matches, eventCategoryRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int eventCode = std::stoi(matches[1]);
            return handleDeleteEventCategory(eventCode);
        } else if (method == "PUT" && std::regex_match(path, matches, eventCategoryRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int eventCode = std::stoi(matches[1]);
            return handleUpdateEventCategory(body, eventCode);
        }
        else if (method == "DELETE" && std::regex_match(path, matches, eventCategoryRegex)) {
            if (!validateSession(sessionToken)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Unauthorized";
                return createJsonResponse(errorResponse.dump(), 401);
            }
            int eventCode = std::stoi(matches[1]);
            return handleDeleteEventCategory(eventCode);
            
        }
        
        
        // 🏠 СТАТУС СЕРВЕРА
        else if (method == "GET" && path == "/status") {
            return handleStatus();
        }
        
        // Если не найден подходящий маршрут
        std::cout << "❌ Маршрут не найден: " << method << " " << path << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Endpoint not found\"}", 404);
        
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в processRequest для клиента " << clientIP << ": " << e.what() << std::endl;
        logSuspiciousActivity(path, "Exception from IP: " + clientIP + " - " + std::string(e.what()));
        return createJsonResponse("{\"success\": false, \"error\": \"Internal server error\"}", 500);
    }
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
               R"({"success":false,"error":"Empty response"})";
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

std::string ApiService::getUserIdFromSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(token);
    if (it != sessions.end()) {
        return it->second.userId;
    }
    
    // Check DB
    Session sess = dbService.getSessionByToken(token);
    if (!sess.token.empty()) {
        return sess.userId;
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

// НОВАЯ ФУНКЦИЯ ДЛЯ ОТЗЫВА СЕССИИ ПО ТОКЕНУ В URL
std::string ApiService::handleRevokeSessionByToken(const std::string& targetToken, const std::string& sessionToken) {
    std::cout << "🔐 Обработка отзыва сессии по токену из URL..." << std::endl;
    std::cout << "🎯 Целевой токен: " << targetToken << std::endl;

    std::string userId = getUserIdFromSession(sessionToken);

    std::cout << "🎯 Отзыв сессии для пользователя: " << userId << std::endl;
    std::cout << "🔑 Целевой токен: " << targetToken << std::endl;
    std::cout << "🔑 Текущий токен: " << sessionToken << std::endl;

    if (targetToken == sessionToken) {
        std::cout << "❌ Пользователь пытается отозвать текущую сессию" << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Cannot revoke current session";
        return createJsonResponse(errorResponse.dump(), 400);
    }

    // Получаем целевую сессию из базы данных
    Session targetSession = dbService.getSessionByToken(targetToken);

    if (targetSession.token.empty()) {
        std::cout << "❌ Сессия не найдена в БД" << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Session not found";
        return createJsonResponse(errorResponse.dump(), 404);
    }

    // Проверяем, что сессия принадлежит текущему пользователю
    if (targetSession.userId != userId) {
        std::cout << "❌ Доступ запрещен: сессия принадлежит другому пользователю" << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Access denied";
        return createJsonResponse(errorResponse.dump(), 403);
    }

    // УДАЛЯЕМ СЕССИЮ ИЗ БАЗЫ ДАННЫХ
    bool deleteSuccess = dbService.deleteSession(targetToken);

    if (deleteSuccess) {
        // УДАЛЯЕМ СЕССИЮ ИЗ ПАМЯТИ
        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions.erase(targetToken);
        }

        std::cout << "✅ Сессия успешно отозвана!" << std::endl;

        json response;
        response["success"] = true;
        response["message"] = "Session revoked successfully";
        return createJsonResponse(response.dump());
    } else {
        std::cout << "❌ Ошибка при удалении сессии из базы данных" << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Failed to revoke session";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}