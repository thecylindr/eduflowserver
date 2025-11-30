#include "api/ApiService.h"
#include "json.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>

using json = nlohmann::json;

bool ApiService::isValidPhoneNumber(const std::string& phoneNumber) {
    if (phoneNumber.length() != 11) {
        return false;
    }
    
    return std::all_of(phoneNumber.begin(), phoneNumber.end(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
    });
}

std::string ApiService::handleRegister(const std::string& body, const std::string& clientInfo) {
    std::string clientIP = "unknown";
    size_t ipPos = clientInfo.find("IP: ");
    if (ipPos != std::string::npos) {
        size_t ipEnd = clientInfo.find(",", ipPos);
        if (ipEnd != std::string::npos) {
            clientIP = clientInfo.substr(ipPos + 4, ipEnd - ipPos - 4);
        }
    }
    
    try {
        json j = json::parse(body);
        std::string username = j["username"];
        std::string email = j["email"];
        std::string password = j["password"];
        std::string firstName = j["firstName"];
        std::string lastName = j["lastName"];
        std::string middleName = j.value("middleName", "");
        std::string phoneNumber = j.value("phoneNumber", "");
        
        if (!phoneNumber.empty()) {
            if (!ApiService::isValidPhoneNumber(phoneNumber)) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Номер телефона должен содержать ровно 11 цифр.";
                return createJsonResponse(errorResponse.dump(), 400);
            }
        }
        
        if (password.length() < 6) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пароль должен содержать не менее 6 символов.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::vector<std::string> russianDomains = {
            "ya.ru", "yandex.ru", "mail.ru", "bk.ru", "list.ru",
            "inbox.ru", "rambler.ru", "russianpost.ru", "mgts.ru"
        };
        
        size_t atPos = email.find('@');
        if (atPos == std::string::npos) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный формат почты.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string domain = email.substr(atPos + 1);
        std::transform(domain.begin(), domain.end(), domain.begin(), ::tolower);
        bool validDomain = false;
        
        for (const auto& russianDomain : russianDomains) {
            if (domain == russianDomain) {
                validDomain = true;
                break;
            }
        }
        
        if (!validDomain) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Регистрация разрешена только с российскими доменами почты.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        User existingUserByEmail = dbService.getUserByEmail(email);
        if (existingUserByEmail.userId != 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь с такой почтой уже существует.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        User existingUserByLogin = dbService.getUserByLogin(username);
        if (existingUserByLogin.userId != 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь с таким логином уже существует.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (!phoneNumber.empty()) {
            User existingUserByPhone = dbService.getUserByPhoneNumber(phoneNumber);
            if (existingUserByPhone.userId != 0) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Пользователь с таким номером телефона уже существует.";
                return createJsonResponse(errorResponse.dump(), 400);
            }
        }
        
        User user;
        user.email = email;
        user.login = username;
        user.passwordHash = hashPassword(password);
        user.firstName = firstName;
        user.lastName = lastName;
        user.middleName = middleName;
        user.phoneNumber = phoneNumber;
        
        if (dbService.addUser(user)) {
            json response;
            response["success"] = true;
            response["message"] = "Регистрация успешна! Теперь вы можете войти в систему.";
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка регистрации. Попробуйте еще раз.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса.";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::hashPassword(const std::string& password) {
    if (password.empty()) {
        return "";
    }
    
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) return "";
    
    const EVP_MD *md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    
    if (!EVP_DigestInit_ex(mdctx, md, NULL)) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    if (!EVP_DigestUpdate(mdctx, password.c_str(), password.length())) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    if (!EVP_DigestFinal_ex(mdctx, hash, &hashLen)) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    
    EVP_MD_CTX_free(mdctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

std::string ApiService::handleLogin(const std::string& body, const std::string& clientInfo) {
    try {
        json j = json::parse(body);
        
        if (!j.contains("login") || j["login"].is_null()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поле логина обязательно и не может быть пустым!";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (!j.contains("password") || j["password"].is_null()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поле пароля обязательно и не может быть пустым!";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string login = j["login"];
        std::string password = j["password"];
        std::string os = j.value("os", "unknown");
        
        if (login.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Логин не может быть пустым.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (password.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пароль не может быть пустым.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string ipAddress = "unknown";
        size_t ipPos = clientInfo.find("IP: ");
        if (ipPos != std::string::npos) {
            size_t ipEnd = clientInfo.find(",", ipPos);
            if (ipEnd != std::string::npos) {
                ipAddress = clientInfo.substr(ipPos + 4, ipEnd - ipPos - 4);
            }
        }
        
        std::string userOS = os;
        
        User user = dbService.getUserByLogin(login);
        if (user.userId == 0) {
            user = dbService.getUserByEmail(login);
        }
        
        if (user.userId == 0 || user.passwordHash != hashPassword(password)) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный логин или пароль.";
            return createJsonResponse(errorResponse.dump(), 401);
        }
        
        Session session;
        session.token = generateSessionToken();
        session.userId = std::to_string(user.userId);
        session.email = user.email;
        session.userOS = userOS;
        session.ipAddress = ipAddress;
        session.createdAt = std::chrono::system_clock::now();
        session.lastActivity = session.createdAt;
        session.expiresAt = session.createdAt + std::chrono::hours(apiConfig.sessionTimeoutHours);
        
        if (dbService.addSession(session)) {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions[session.token] = session;
            
            json response;
            response["success"] = true;
            response["token"] = session.token;
            response["user"] = {
                {"userId", user.userId},
                {"login", user.login},
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
            errorResponse["error"] = "Ошибка создания сессии.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Внутренняя ошибка сервера: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleForgotPassword(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string email = j["email"];
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            json response;
            response["success"] = true;
            response["message"] = "Если email существует, код сброса пароля был отправлен.";
            return createJsonResponse(response.dump());
        }
        
        std::string resetToken = generateSessionToken();
        {
            std::lock_guard<std::mutex> lock(passwordResetMutex);
            passwordResetTokens[resetToken] = PasswordResetToken{
                email,
                std::chrono::system_clock::now()
            };
        }
        
        json response;
        response["success"] = true;
        response["message"] = "Код сброса сгенерирован.";
        response["resetToken"] = resetToken;
        return createJsonResponse(response.dump());
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный запрос";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleResetPassword(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string resetToken = j["resetToken"];
        std::string newPassword = j["newPassword"];
        
        if (newPassword.length() < 6) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пароль должен содержать не менее 6 символов.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string email;
        {
            std::lock_guard<std::mutex> lock(passwordResetMutex);
            auto it = passwordResetTokens.find(resetToken);
            if (it == passwordResetTokens.end()) {
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Неверный или просроченный токен сброса";
                return createJsonResponse(errorResponse.dump(), 400);
            }
            
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.createdAt);
            if (duration.count() > 60) {
                passwordResetTokens.erase(it);
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Токен сброса просрочен";
                return createJsonResponse(errorResponse.dump(), 400);
            }
            
            email = it->second.email;
            passwordResetTokens.erase(it);
        }
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь не найден";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        user.passwordHash = hashPassword(newPassword);
        if (dbService.updateUser(user)) {
            json response;
            response["success"] = true;
            response["message"] = "Пароль успешно сброшен";
            return createJsonResponse(response.dump());
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка сброса пароля";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный запрос";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleLogout(const std::string& sessionToken, const std::string& clientInfo) {
    std::string clientIP = "unknown";
    size_t ipPos = clientInfo.find("IP: ");
    if (ipPos != std::string::npos) {
        size_t ipEnd = clientInfo.find(",", ipPos);
        if (ipEnd != std::string::npos) {
            clientIP = clientInfo.substr(ipPos + 4, ipEnd - ipPos - 4);
        }
    }
    
    if (!sessionToken.empty()) {
        dbService.deleteSession(sessionToken);
        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions.erase(sessionToken);
        }
    }
    
    json response;
    response["success"] = true;
    response["message"] = "Выход с учётной записи успешно осуществлён";
    return createJsonResponse(response.dump());
}