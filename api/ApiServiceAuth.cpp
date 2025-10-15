#include "api/ApiService.h"
#include "json.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>

using json = nlohmann::json;

std::string ApiService::handleRegister(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string username = j["username"];
        std::string email = j["email"];
        std::string password = j["password"];
        std::string firstName = j["firstName"];
        std::string lastName = j["lastName"];
        std::string middleName = j.value("middleName", "");
        std::string phoneNumber = j.value("phoneNumber", "");
        
        // Проверка российских доменов почты
        std::vector<std::string> russianDomains = {
            "ya.ru", "yandex.ru", "mail.ru", "bk.ru", "list.ru", 
            "inbox.ru", "rambler.ru", "russianpost.ru", "mgts.ru"
        };
        
        size_t atPos = email.find('@');
        if (atPos == std::string::npos) {
            return createJsonResponse("{\"error\": \"Неверный формат почты\"}", 400);
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
            return createJsonResponse("{\"error\": \"Регистрация разрешена только с российскими доменами почты\"}", 400);
        }
        
        // Проверка существования пользователя
        User existingUserByEmail = dbService.getUserByEmail(email);
        if (existingUserByEmail.userId != 0) {
            return createJsonResponse("{\"error\": \"Пользователь с такой почтой уже существует\"}", 400);
        }
        
        User existingUserByLogin = dbService.getUserByLogin(username);
        if (existingUserByLogin.userId != 0) {
            return createJsonResponse("{\"error\": \"Пользователь с таким логином уже существует\"}", 400);
        }
        
        if (!phoneNumber.empty()) {
            User existingUserByPhone = dbService.getUserByPhoneNumber(phoneNumber);
            if (existingUserByPhone.userId != 0) {
                return createJsonResponse("{\"error\": \"Пользователь с таким номером телефона уже существует\"}", 400);
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
        
        if (dbService.addUser(user)) {
            return createJsonResponse("{\"message\": \"Регистрация успешна! Теперь вы можете войти в систему.\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Ошибка регистрации. Попробуйте еще раз.\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Неверный формат запроса\"}", 400);
    }
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


std::string ApiService::handleLogin(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string emailOrLogin = j["email"]; // Поле теперь может содержать email или логин
        std::string password = j["password"];
        
        //std::cout << "🔐 Attempting login for: " << emailOrLogin << std::endl;
        
        User user;
        
        // Сначала пытаемся найти пользователя по email
        user = dbService.getUserByEmail(emailOrLogin);
        
        // Если не нашли по email, ищем по логину
        if (user.userId == 0) {
            user = dbService.getUserByLogin(emailOrLogin);
            //std::cout << "🔄 Trying login lookup for: " << emailOrLogin << std::endl;
        }
        
        // ДЕБАГ: проверяем что получили
        //std::cout << "📋 User data - ID: " << user.userId 
                //   << ", Email: " << user.email 
                //   << ", Login: " << user.login
                //   << ", Hash length: " << user.passwordHash.length() << std::endl;
        
        if (user.userId == 0) {
            //std::cout << "❌ User not found: " << emailOrLogin << std::endl;
            return createJsonResponse("{\"error\": \"Пользователь с такими учетными данными не найден.\"}", 401);
        }
        
        std::string hashedPassword = hashPassword(password);
        //std::cout << "🔑 Password check - Input hash: " << hashedPassword 
                  //<< ", Stored hash: " << user.passwordHash << std::endl;
        
        if (user.passwordHash != hashedPassword) {
            //std::cout << "❌ Password mismatch for user: " << emailOrLogin << std::endl;
            return createJsonResponse("{\"error\": \"Неверный пароль.\"}", 401);
        }
        
        std::string sessionToken = generateSessionToken();
        //std::cout << "✅ Generated session token: " << sessionToken << std::endl;
        
        Session session;
        session.userId = std::to_string(user.userId);
        session.email = user.email;
        session.createdAt = std::chrono::system_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions[sessionToken] = session;
        }
        
        json response;
        response["message"] = "Вход выполнен успешно";
        response["token"] = sessionToken;
        response["user"] = {
            {"userId", user.userId},
            {"email", user.email},
            {"firstName", user.firstName},
            {"lastName", user.lastName},
            {"login", user.login}
        };
        
        //std::cout << "✅ Login successful for: " << emailOrLogin << std::endl;
        return createJsonResponse(response.dump());
        
    } catch (const std::exception& e) {
        //std::cout << "💥 EXCEPTION in handleLogin: " << e.what() << std::endl;
        return createJsonResponse("{\"error\": \"Ошибка сервера при авторизации.\"}", 500);
    }
}

std::string ApiService::handleForgotPassword(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string email = j["email"];
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            return createJsonResponse("{\"message\": \"Если email существует, код сброса был отправлен\"}");
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
        response["message"] = "Код сброса сгенерирован";
        response["resetToken"] = resetToken;
        
        return createJsonResponse(response.dump());
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Неверный запрос.\"}", 400);
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
                return createJsonResponse("{\"error\": \"Неверный или просроченный токен сброса\"}", 400);
            }
            
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.createdAt);
            if (duration.count() > 60) {
                passwordResetTokens.erase(it);
                return createJsonResponse("{\"error\": \"Токен сброса просрочен\"}", 400);
            }
            
            email = it->second.email;
            passwordResetTokens.erase(it);
        }
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            return createJsonResponse("{\"error\": \"Пользователь не найден\"}", 404);
        }
        
        user.passwordHash = hashPassword(newPassword);
        if (dbService.updateUser(user)) {
            return createJsonResponse("{\"message\": \"Пароль успешно сброшен\"}");
        } else {
            return createJsonResponse("{\"error\": \"Ошибка сброса пароля\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Неверный запрос.\"}", 400);
    }
}

std::string ApiService::handleLogout(const std::string& sessionToken) {
    if (!sessionToken.empty()) {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        sessions.erase(sessionToken);
    }
    
    return createJsonResponse("{\"message\": \"Выход с учётной записи успешно осуществлён.\"}");
}