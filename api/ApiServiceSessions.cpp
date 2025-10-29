#include "api/ApiService.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// Сериализация сессий в JSON
void ApiService::saveSessionsToFile() {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    
    json sessionsJson;
    for (const auto& [token, session] : sessions) {
        json sessionJson;
        sessionJson["userId"] = session.userId.empty() ? "" : session.userId;
        sessionJson["email"] = session.email.empty() ? "" : session.email;
        sessionJson["createdAt"] = std::chrono::duration_cast<std::chrono::seconds>(
            session.createdAt.time_since_epoch()).count();
        sessionJson["lastActivity"] = std::chrono::duration_cast<std::chrono::seconds>(
            session.lastActivity.time_since_epoch()).count();
        
        sessionsJson[token] = sessionJson;
    }
    
    std::ofstream file("sessions.json");
    if (file.is_open()) {
        file << sessionsJson.dump(4);
        file.close();
        std::cout << "💾 Sessions saved to file: " << sessions.size() << " sessions" << std::endl;
    } else {
        std::cout << "❌ Failed to save sessions to file" << std::endl;
    }
}

// Загрузка сессий из файла
void ApiService::loadSessionsFromFile() {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    
    std::ifstream file("sessions.json");
    if (!file.is_open()) {
        std::cout << "📝 No existing sessions file found" << std::endl;
        return;
    }
    
    try {
        json sessionsJson;
        file >> sessionsJson;
        file.close();
        
        auto now = std::chrono::system_clock::now();
        size_t loadedCount = 0;
        
        for (auto& [token, sessionJson] : sessionsJson.items()) {
            Session session;
            session.userId = sessionJson.value("userId", "");
            session.email = sessionJson.value("email", "");
            
            auto createdAtSeconds = std::chrono::seconds(sessionJson["createdAt"]);
            session.createdAt = std::chrono::system_clock::time_point(createdAtSeconds);
            
            auto lastActivitySeconds = std::chrono::seconds(sessionJson["lastActivity"]);
            session.lastActivity = std::chrono::system_clock::time_point(lastActivitySeconds);
            
            // Проверяем по последней активности, а не по созданию
            auto duration = std::chrono::duration_cast<std::chrono::hours>(now - session.lastActivity);
            if (duration.count() <= apiConfig.sessionTimeoutHours) {
                sessions[token] = session;
                loadedCount++;
            } else {
                // Сессия истекла, не загружаем
            }
        }
        
        std::cout << "📥 Sessions loaded from file: " << loadedCount << " valid sessions" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Error loading sessions: " << e.what() << std::endl;
    }
}

// Очистка устаревших сессий
void ApiService::cleanupExpiredSessions() {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    
    auto now = std::chrono::system_clock::now();
    size_t initialSize = sessions.size();
    
    auto it = sessions.begin();
    while (it != sessions.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - it->second.lastActivity);
        if (duration.count() > apiConfig.sessionTimeoutHours) {
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
    
    if (initialSize != sessions.size()) {
        std::cout << "🧹 Cleaned up " << (initialSize - sessions.size()) 
                  << " expired sessions, remaining: " << sessions.size() << std::endl;
    }
}

// Получение информации о сессии - ПОЛНОСТЬЮ ПЕРЕПИСАННЫЙ МЕТОД
std::string ApiService::getSessionInfo(const std::string& sessionToken) {
    if (sessionToken.empty()) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Session token is required";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    // Используем validateSession для проверки (она уже использует мьютекс внутри)
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid session";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    // После validateSession сессия гарантированно существует и валидна
    // Нужно только защитить доступ к данным сессии
    std::lock_guard<std::mutex> lock(sessionsMutex);
    
    auto it = sessions.find(sessionToken);
    if (it == sessions.end()) {
        // Это не должно происходить после validateSession, но на всякий случай
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid session";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - it->second.createdAt);
    auto inactive = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.lastActivity);
    
    json response;
    response["success"] = true;
    response["userId"] = it->second.userId.empty() ? "" : it->second.userId;
    response["email"] = it->second.email.empty() ? "" : it->second.email;
    response["ageHours"] = age.count();
    response["inactiveMinutes"] = inactive.count();
    response["timeoutHours"] = apiConfig.sessionTimeoutHours;
    response["remainingHours"] = (apiConfig.sessionTimeoutHours - age.count());
    
    return createJsonResponse(response.dump());
}