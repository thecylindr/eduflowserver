#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

std::string ApiService::handleUpdateProfile(const std::string& body, const std::string& sessionToken) {
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
        
        bool updated = false;
        
        if (j.contains("firstName") && !j["firstName"].is_null()) {
            user.firstName = j["firstName"];
            updated = true;
        }
        
        if (j.contains("lastName") && !j["lastName"].is_null()) {
            user.lastName = j["lastName"];
            updated = true;
        }
        
        if (j.contains("middleName") && !j["middleName"].is_null()) {
            user.middleName = j["middleName"];
            updated = true;
        }
        
        if (j.contains("email") && !j["email"].is_null()) {
            std::string newEmail = j["email"];
            User existingUser = dbService.getUserByEmail(newEmail);
            if (existingUser.userId != 0 && existingUser.userId != user.userId) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Email уже используется другим пользователем.";
                return createJsonResponse(errorResponse.dump(), 400);
            }
            user.email = newEmail;
            updated = true;
        }
        
        if (j.contains("phoneNumber") && !j["phoneNumber"].is_null()) {
            std::string newPhone = j["phoneNumber"];
            
            if (!newPhone.empty()) {
                if (!ApiService::isValidPhoneNumber(newPhone)) {
                    json errorResponse;
                    errorResponse["success"] = false;
                    errorResponse["error"] = "Номер телефона должен содержать ровно 11 цифр.";
                    return createJsonResponse(errorResponse.dump(), 400);
                }
                
                User existingUser = dbService.getUserByPhoneNumber(newPhone);
                if (existingUser.userId != 0 && existingUser.userId != user.userId) {
                    json errorResponse;
                    errorResponse["success"] = false;
                    errorResponse["error"] = "Номер телефона уже используется другим пользователем.";
                    return createJsonResponse(errorResponse.dump(), 400);
                }
            }
            user.phoneNumber = newPhone;
            updated = true;
        }
        
        if (!updated) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Нет данных для обновления.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (dbService.updateUser(user)) {
            json response;
            response["success"] = true;
            response["message"] = "Профиль успешно обновлен.";
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
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при обновлении профиля.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleChangePassword(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        std::string currentPassword = j["currentPassword"];
        std::string newPassword = j["newPassword"];
        std::string userId = getUserIdFromSession(sessionToken);
        
        if (userId.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Недействительная сессия";
            return createJsonResponse(errorResponse.dump(), 401);
        }
        
        User user = dbService.getUserById(std::stoi(userId));
        if (user.userId == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь не найден.";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        std::string currentHash = hashPassword(currentPassword);
        if (currentHash != user.passwordHash) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный текущий пароль";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        user.passwordHash = hashPassword(newPassword);
        if (dbService.updateUser(user)) {
            {
                std::lock_guard<std::mutex> lock(sessionsMutex);
                auto it = sessions.begin();
                while (it != sessions.end()) {
                    if (it->second.userId == userId && it->first != sessionToken) {
                        dbService.deleteSession(it->first);
                        it = sessions.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            
            json response;
            response["success"] = true;
            response["message"] = "Пароль успешно изменен. Все другие сессии были отозваны.";
            return createJsonResponse(response.dump());
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при изменении пароля";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}