#include "api/ApiService.h"
#include "json.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>

using json = nlohmann::json;

std::string ApiService::handleRegister(const std::string& body, const std::string& clientInfo) {
    // Используем clientInfo для логирования
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
        
        std::cout << "👤 Registration attempt from " << clientIP << " - Username: " << username << ", Email: " << email << std::endl;
        
        // ДОБАВЛЕНО: Проверка минимальной длины пароля
        if (password.length() < 6) {
            std::cout << "❌ Password too short from " << clientIP << ": " << email << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пароль должен содержать не менее 6 символов";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        // Проверка российских доменов почты
        std::vector<std::string> russianDomains = {
            "ya.ru", "yandex.ru", "mail.ru", "bk.ru", "list.ru",
            "inbox.ru", "rambler.ru", "russianpost.ru", "mgts.ru"
        };
        
        size_t atPos = email.find('@');
        if (atPos == std::string::npos) {
            std::cout << "❌ Invalid email format from " << clientIP << ": " << email << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный формат почты";
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
            std::cout << "❌ Non-Russian email domain from " << clientIP << ": " << domain << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Регистрация разрешена только с российскими доменами почты";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        // Проверка существования пользователя
        User existingUserByEmail = dbService.getUserByEmail(email);
        if (existingUserByEmail.userId != 0) {
            std::cout << "❌ User with email already exists from " << clientIP << ": " << email << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь с такой почтой уже существует";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        User existingUserByLogin = dbService.getUserByLogin(username);
        if (existingUserByLogin.userId != 0) {
            std::cout << "❌ User with login already exists from " << clientIP << ": " << username << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь с таким логином уже существует";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (!phoneNumber.empty()) {
            User existingUserByPhone = dbService.getUserByPhoneNumber(phoneNumber);
            if (existingUserByPhone.userId != 0) {
                std::cout << "❌ User with phone already exists from " << clientIP << ": " << phoneNumber << std::endl;
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Пользователь с таким номером телефона уже существует";
                return createJsonResponse(errorResponse.dump(), 400);
            }
        }
        
        // Создание пользователя
        User user;
        user.email = email;
        user.login = username;
        user.passwordHash = hashPassword(password);
        user.firstName = firstName;
        user.lastName = lastName;
        user.middleName = middleName;
        user.phoneNumber = phoneNumber;
        
        std::cout << "🔑 Password hash generated for " << clientIP << ", length: " << user.passwordHash.length() << std::endl;
        
        if (dbService.addUser(user)) {
            std::cout << "✅ User registered successfully from " << clientIP << ": " << email << std::endl;
            json response;
            response["success"] = true;
            response["message"] = "Регистрация успешна! Теперь вы можете войти в систему.";
            return createJsonResponse(response.dump(), 201);
        } else {
            std::cout << "❌ Failed to add user to database from " << clientIP << ": " << email << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка регистрации. Попробуйте еще раз.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleRegister from " << clientIP << ": " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса";
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
        
        // Валидация обязательных полей
        if (!j.contains("login") || j["login"].is_null()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поле 'login' обязательно и не может быть пустым";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (!j.contains("password") || j["password"].is_null()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поле 'password' обязательно и не может быть пустым";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string login = j["login"];
        std::string password = j["password"];
        std::string os = j.value("os", "unknown"); // Получаем ОС из запроса
        
        // Дополнительная валидация
        if (login.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Логин не может быть пустым";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (password.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пароль не может быть пустым";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        // Извлекаем IP из clientInfo
        std::string ipAddress = "unknown";
        size_t ipPos = clientInfo.find("IP: ");
        if (ipPos != std::string::npos) {
            size_t ipEnd = clientInfo.find(",", ipPos);
            if (ipEnd != std::string::npos) {
                ipAddress = clientInfo.substr(ipPos + 4, ipEnd - ipPos - 4);
            }
        }
        
        // Используем ОС из запроса вместо User-Agent
        std::string userOS = os;
        
        // ИЗМЕНЕНО: Поиск пользователя по логину ИЛИ email
        User user = dbService.getUserByLogin(login);
        if (user.userId == 0) {
            // Если не нашли по логину, пробуем найти по email
            user = dbService.getUserByEmail(login);
        }
        
        // ИЗМЕНЕНО: Русская ошибка аутентификации
        if (user.userId == 0 || user.passwordHash != hashPassword(password)) {
            std::cout << "❌ Failed login attempt from " << ipAddress << " for user: " << login << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный логин или пароль";  // ИЗМЕНЕНО НА РУССКИЙ
            return createJsonResponse(errorResponse.dump(), 401);
        }
        
        // Создаем сессию с информацией об ОС и IP
        Session session;
        session.token = generateSessionToken();
        session.userId = std::to_string(user.userId);
        session.email = user.email;
        session.userOS = userOS; // Теперь здесь ОС из Qt.platform.os
        session.ipAddress = ipAddress;
        session.createdAt = std::chrono::system_clock::now();
        session.lastActivity = session.createdAt;
        session.expiresAt = session.createdAt + std::chrono::hours(apiConfig.sessionTimeoutHours);
        
        // Сохраняем в базу и память
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
            
            std::cout << "✅ User logged in from " << ipAddress << ": " << login 
                      << " with OS: " << userOS << std::endl;
            
            return createJsonResponse(response.dump());
        } else {
            std::cout << "❌ Failed to create session for " << ipAddress << ": " << login << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Failed to create session";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleLogin: " << e.what() << std::endl;
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
        std::cout << "🔑 Forgot password request for: " << email << std::endl;
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            std::cout << "⚠️  User not found for password reset: " << email << std::endl;
            json response;
            response["success"] = true;
            response["message"] = "Если email существует, код сброса был отправлен";
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
        
        std::cout << "✅ Password reset token generated for: " << email << std::endl;
        json response;
        response["success"] = true;
        response["message"] = "Код сброса сгенерирован";
        response["resetToken"] = resetToken;
        return createJsonResponse(response.dump());
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleForgotPassword: " << e.what() << std::endl;
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
        std::cout << "🔑 Password reset attempt with token: " << resetToken.substr(0, 16) << "..." << std::endl;
        
        // ДОБАВЛЕНО: Проверка минимальной длины пароля
        if (newPassword.length() < 6) {
            std::cout << "❌ New password too short for reset token: " << resetToken.substr(0, 16) << "..." << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пароль должен содержать не менее 6 символов";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string email;
        {
            std::lock_guard<std::mutex> lock(passwordResetMutex);
            auto it = passwordResetTokens.find(resetToken);
            if (it == passwordResetTokens.end()) {
                std::cout << "❌ Invalid reset token: " << resetToken.substr(0, 16) << "..." << std::endl;
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Неверный или просроченный токен сброса";
                return createJsonResponse(errorResponse.dump(), 400);
            }
            
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.createdAt);
            if (duration.count() > 60) {
                std::cout << "❌ Expired reset token: " << resetToken.substr(0, 16) << "..." << std::endl;
                passwordResetTokens.erase(it);
                json errorResponse;
                errorResponse["success"] = false;
                errorResponse["error"] = "Токен сброса просрочен";
                return createJsonResponse(errorResponse.dump(), 400);
            }
            
            email = it->second.email;
            passwordResetTokens.erase(it);
        }
        
        std::cout << "✅ Valid reset token for: " << email << std::endl;
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            std::cout << "❌ User not found for password reset: " << email << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь не найден";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        user.passwordHash = hashPassword(newPassword);
        if (dbService.updateUser(user)) {
            std::cout << "✅ Password reset successful for: " << email << std::endl;
            json response;
            response["success"] = true;
            response["message"] = "Пароль успешно сброшен";
            return createJsonResponse(response.dump());
        } else {
            std::cout << "❌ Failed to update password for: " << email << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка сброса пароля";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleResetPassword: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный запрос";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleLogout(const std::string& sessionToken, const std::string& clientInfo) {
    // Используем clientInfo для логирования
    std::string clientIP = "unknown";
    size_t ipPos = clientInfo.find("IP: ");
    if (ipPos != std::string::npos) {
        size_t ipEnd = clientInfo.find(",", ipPos);
        if (ipEnd != std::string::npos) {
            clientIP = clientInfo.substr(ipPos + 4, ipEnd - ipPos - 4);
        }
    }
    
    std::cout << "🚪 Logout request from " << clientIP << ", token: " << (sessionToken.empty() ? "empty" : sessionToken.substr(0, 16) + "...") << std::endl;
    
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