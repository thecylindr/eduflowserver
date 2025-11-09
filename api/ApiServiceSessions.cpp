#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

void ApiService::loadSessionsFromDB() {
    auto activeSessions = dbService.getAllActiveSessions();
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        sessions.clear();
        for (const auto& sess : activeSessions) {
            sessions[sess.token] = sess;
        }
    }
    std::cout << "📥 Loaded " << activeSessions.size() << " active sessions from DB" << std::endl;
}

std::string ApiService::getSessionInfo(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(token);
    if (it != sessions.end()) {
        json sessionJson;
        sessionJson["userId"] = it->second.userId;
        sessionJson["email"] = it->second.email;
        sessionJson["createdAt"] = std::chrono::duration_cast<std::chrono::seconds>(
            it->second.createdAt.time_since_epoch()).count();
        sessionJson["lastActivity"] = std::chrono::duration_cast<std::chrono::seconds>(
            it->second.lastActivity.time_since_epoch()).count();
        return sessionJson.dump();
    }
    return "{}";
}

std::string ApiService::handleGetSessions(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::string userId = getUserIdFromSession(sessionToken);
    auto userSessions = dbService.getSessionsByUserId(userId);
    json sessionsArray = json::array();
    auto now = std::chrono::system_clock::now();
    
    for (const auto& session : userSessions) {
        if (now > session.expiresAt) continue;
        
        auto age = std::chrono::duration_cast<std::chrono::hours>(now - session.createdAt);
        auto inactive = std::chrono::duration_cast<std::chrono::minutes>(now - session.lastActivity);
        
        json sessionJson;
        sessionJson["token"] = session.token;
        sessionJson["email"] = session.email;
        sessionJson["userOS"] = session.userOS;
        sessionJson["ipAddress"] = session.ipAddress;
        sessionJson["createdAt"] = std::chrono::duration_cast<std::chrono::seconds>(
            session.createdAt.time_since_epoch()).count();
        sessionJson["lastActivity"] = std::chrono::duration_cast<std::chrono::seconds>(
            session.lastActivity.time_since_epoch()).count();
        sessionJson["ageHours"] = age.count();
        sessionJson["inactiveMinutes"] = inactive.count();
        sessionJson["isCurrent"] = (session.token == sessionToken);
        
        sessionsArray.push_back(sessionJson);
    }
    
    json response;
    response["success"] = true;
    response["data"] = sessionsArray;
    return createJsonResponse(response.dump());
}

std::string ApiService::handleRevokeSession(const std::string& body, const std::string& sessionToken) {
    std::cout << "🔐 Обработка отзыва сессии..." << std::endl;
    
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::cout << "📦 Длина тела запроса: " << body.length() << " байт" << std::endl;
    
    // ПРОВЕРКА НА ПУСТОЕ ТЕЛО ЗАПРОСА
    if (body.empty()) {
        std::cout << "❌ ПУСТОЕ ТЕЛО ЗАПРОСА!" << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Empty request body - token is required";
        return createJsonResponse(errorResponse.dump(), 400);
    }
    
    try {
        json j = json::parse(body);
        std::cout << "✅ JSON успешно распарсен" << std::endl;
        
        if (!j.contains("token") || j["token"].is_null() || j["token"].empty()) {
            std::cout << "❌ Токен отсутствует или пустой в JSON" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Token is required and cannot be empty";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string targetToken = j["token"];
        std::string userId = getUserIdFromSession(sessionToken);
        
        std::cout << "🎯 Отзыв сессии для пользователя: " << userId << std::endl;
        std::cout << "🔑 Целевой токен: " << targetToken.substr(0, 16) << "..." << std::endl;
        std::cout << "🔑 Текущий токен: " << sessionToken.substr(0, 16) << "..." << std::endl;
        
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
    } catch (const json::parse_error& e) {
        std::cout << "💥 Ошибка парсинга JSON: " << e.what() << std::endl;
        std::cout << "📦 Problematic body: " << body << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid JSON format: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в handleRevokeSession: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Server error: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

bool ApiService::validateSession(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = sessions.find(token);
    if (it == sessions.end()) {
        // Check DB if not in memory
        Session sess = dbService.getSessionByToken(token);
        if (sess.token.empty()) {
            return false;
        }
        auto now = std::chrono::system_clock::now();
        if (now > sess.expiresAt) {
            dbService.deleteSession(token);
            return false;
        }
        sessions[token] = sess;
        it = sessions.find(token);
    }
    
    auto now = std::chrono::system_clock::now();
    if (now > it->second.expiresAt) {
        dbService.deleteSession(token);
        sessions.erase(it);
        return false;
    }
    
    // Update last activity and expires
    auto newLast = now;
    auto newExpires = now + std::chrono::hours(apiConfig.sessionTimeoutHours);
    it->second.lastActivity = newLast;
    it->second.expiresAt = newExpires;
    
    // Update in DB
    dbService.updateSessionLastActivity(token, newLast, newExpires);
    return true;
}

bool ApiService::validateTokenInDatabase(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    
    // Проверяем в базе данных
    Session sess = dbService.getSessionByToken(token);
    if (sess.token.empty()) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    if (now > sess.expiresAt) {
        dbService.deleteSession(token);
        return false;
    }
    
    // Обновляем время последней активности
    dbService.updateSessionLastActivity(token, now, now + std::chrono::hours(apiConfig.sessionTimeoutHours));
    return true;
}


void ApiService::cleanupExpiredSessions() {
    auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(sessionsMutex);
    for (auto it = sessions.begin(); it != sessions.end(); ) {
        if (now > it->second.expiresAt) {
            dbService.deleteSession(it->first);
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
}