#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

std::string ApiService::handleUpdateProfile(const std::string& body, const std::string& sessionToken) {
    std::cout << "🔄 Обработка обновления профиля..." << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;

    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        std::string userId = getUserIdFromSession(sessionToken);
        
        if (userId.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Invalid session";
            return createJsonResponse(errorResponse.dump(), 401);
        }
        
        User user = dbService.getUserById(std::stoi(userId));
        if (user.userId == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "User not found";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        std::cout << "👤 Обновление профиля пользователя ID: " << userId << std::endl;
        
        bool updated = false;
        
        // Обновляем поля, если они переданы
        if (j.contains("firstName") && !j["firstName"].is_null()) {
            user.firstName = j["firstName"];
            updated = true;
            std::cout << "✅ Обновлено имя: " << user.firstName << std::endl;
        }
        
        if (j.contains("lastName") && !j["lastName"].is_null()) {
            user.lastName = j["lastName"];
            updated = true;
            std::cout << "✅ Обновлена фамилия: " << user.lastName << std::endl;
        }
        
        if (j.contains("middleName") && !j["middleName"].is_null()) {
            user.middleName = j["middleName"];
            updated = true;
            std::cout << "✅ Обновлено отчество: " << user.middleName << std::endl;
        }
        
        if (j.contains("email") && !j["email"].is_null()) {
            std::string newEmail = j["email"];
            // Проверяем, не занят ли email другим пользователем
            User existingUser = dbService.getUserByEmail(newEmail);
            if (existingUser.userId != 0 && existingUser.userId != user.userId) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Email уже используется другим пользователем";
                return createJsonResponse(errorResponse.dump(), 400);
            }
            user.email = newEmail;
            updated = true;
            std::cout << "✅ Обновлен email: " << user.email << std::endl;
        }
        
        if (j.contains("phoneNumber") && !j["phoneNumber"].is_null()) {
            std::string newPhone = j["phoneNumber"];
            // Проверяем, не занят ли телефон другим пользователем
            if (!newPhone.empty()) {
                User existingUser = dbService.getUserByPhoneNumber(newPhone);
                if (existingUser.userId != 0 && existingUser.userId != user.userId) {
                    json errorResponse;
                    errorResponse["success"] = false;
                    errorResponse["error"] = "Номер телефона уже используется другим пользователем";
                    return createJsonResponse(errorResponse.dump(), 400);
                }
            }
            user.phoneNumber = newPhone;
            updated = true;
            std::cout << "✅ Обновлен номер телефона: " << user.phoneNumber << std::endl;
        }
        
        if (!updated) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Нет данных для обновления";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (dbService.updateUser(user)) {
            std::cout << "✅ Профиль успешно обновлен" << std::endl;
            
            json response;
            response["success"] = true;
            response["message"] = "Профиль успешно обновлен";
            response["data"] = {
                {"userId", user.userId},
                {"email", user.email},
                {"firstName", user.firstName},
                {"lastName", user.lastName},
                {"middleName", user.middleName},
                {"phoneNumber", user.phoneNumber}
            };
            
            return createJsonResponse(response.dump());
        } else {
            std::cout << "❌ Ошибка при обновлении профиля" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при обновлении профиля";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в handleUpdateProfile: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleChangePassword(const std::string& body, const std::string& sessionToken) {
    std::cout << "🔄 Обработка смены пароля..." << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;

    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        std::string userId = getUserIdFromSession(sessionToken);
        
        if (userId.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Invalid session";
            return createJsonResponse(errorResponse.dump(), 401);
        }
        
        if (!j.contains("currentPassword") || !j.contains("newPassword")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Требуются currentPassword и newPassword";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string currentPassword = j["currentPassword"];
        std::string newPassword = j["newPassword"];
        
        if (newPassword.length() < 6) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Новый пароль должен содержать не менее 6 символов";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        User user = dbService.getUserById(std::stoi(userId));
        if (user.userId == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "User not found";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        std::cout << "🔐 Проверка текущего пароля для пользователя ID: " << userId << std::endl;
        
        // Проверяем текущий пароль
        std::string currentPasswordHash = hashPassword(currentPassword);
        if (currentPasswordHash != user.passwordHash) {
            std::cout << "❌ Неверный текущий пароль" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный текущий пароль";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        // Хэшируем новый пароль и обновляем
        user.passwordHash = hashPassword(newPassword);
        
        if (dbService.updateUser(user)) {
            std::cout << "✅ Пароль успешно изменен" << std::endl;
            
            // УДАЛЯЕМ ВСЕ СЕССИИ, КРОМЕ ТЕКУЩЕЙ
            {
                std::lock_guard<std::mutex> lock(sessionsMutex);
                auto it = sessions.begin();
                while (it != sessions.end()) {
                    if (it->second.userId == userId && it->first != sessionToken) {
                        std::cout << "🗑️ Удаление сессии: " << it->first.substr(0, 16) << "..." << std::endl;
                        it = sessions.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            
            std::cout << "✅ Все другие сессии удалены" << std::endl;
            
            json response;
            response["success"] = true;
            response["message"] = "Пароль успешно изменен. Все другие сессии были отозваны.";
            
            return createJsonResponse(response.dump());
        } else {
            std::cout << "❌ Ошибка при изменении пароля" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при изменении пароля";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в handleChangePassword: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleGetSessions(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::string userId = getUserIdFromSession(sessionToken);
    if (userId.empty()) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid session";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::lock_guard<std::mutex> lock(sessionsMutex);
    json sessionsArray = json::array();
    
    auto now = std::chrono::system_clock::now();
    
    for (const auto& [token, session] : sessions) {
        if (session.userId == userId) {
            auto age = std::chrono::duration_cast<std::chrono::hours>(now - session.createdAt);
            auto inactive = std::chrono::duration_cast<std::chrono::minutes>(now - session.lastActivity);
            
            json sessionJson;
            sessionJson["token"] = token;
            sessionJson["email"] = session.email;
            sessionJson["createdAt"] = std::chrono::duration_cast<std::chrono::seconds>(session.createdAt.time_since_epoch()).count();
            sessionJson["lastActivity"] = std::chrono::duration_cast<std::chrono::seconds>(session.lastActivity.time_since_epoch()).count();
            sessionJson["ageHours"] = age.count();
            sessionJson["inactiveMinutes"] = inactive.count();
            sessionJson["isCurrent"] = (token == sessionToken);
            
            sessionsArray.push_back(sessionJson);
        }
    }
    
    json response;
    response["success"] = true;
    response["data"] = sessionsArray;
    
    return createJsonResponse(response.dump());
}

// Добавить метод для отзыва сессии
std::string ApiService::handleRevokeSession(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        
        if (!j.contains("token")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Token is required";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string targetToken = j["token"];
        std::string userId = getUserIdFromSession(sessionToken);
        
        if (targetToken == sessionToken) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Cannot revoke current session";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::lock_guard<std::mutex> lock(sessionsMutex);
        auto it = sessions.find(targetToken);
        if (it != sessions.end() && it->second.userId == userId) {
            sessions.erase(it);
            std::cout << "✅ Сессия отозвана: " << targetToken.substr(0, 16) << "..." << std::endl;
            
            json response;
            response["success"] = true;
            response["message"] = "Session revoked successfully";
            return createJsonResponse(response.dump());
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Session not found or access denied";
            return createJsonResponse(errorResponse.dump(), 404);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid request format";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}