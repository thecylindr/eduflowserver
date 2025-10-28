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
        
        std::cout << "👤 Registration attempt - Username: " << username << ", Email: " << email << std::endl;
        
        // Проверка российских доменов почты
        std::vector<std::string> russianDomains = {
            "ya.ru", "yandex.ru", "mail.ru", "bk.ru", "list.ru", 
            "inbox.ru", "rambler.ru", "russianpost.ru", "mgts.ru"
        };
        
        size_t atPos = email.find('@');
        if (atPos == std::string::npos) {
            std::cout << "❌ Invalid email format: " << email << std::endl;
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
            std::cout << "❌ Non-Russian email domain: " << domain << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Регистрация разрешена только с российскими доменами почты";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        // Проверка существования пользователя
        User existingUserByEmail = dbService.getUserByEmail(email);
        if (existingUserByEmail.userId != 0) {
            std::cout << "❌ User with email already exists: " << email << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь с такой почтой уже существует";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        User existingUserByLogin = dbService.getUserByLogin(username);
        if (existingUserByLogin.userId != 0) {
            std::cout << "❌ User with login already exists: " << username << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь с таким логином уже существует";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (!phoneNumber.empty()) {
            User existingUserByPhone = dbService.getUserByPhoneNumber(phoneNumber);
            if (existingUserByPhone.userId != 0) {
                std::cout << "❌ User with phone already exists: " << phoneNumber << std::endl;
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
        
        std::cout << "🔑 Password hash generated, length: " << user.passwordHash.length() << std::endl;
        
        if (dbService.addUser(user)) {
            std::cout << "✅ User registered successfully: " << email << std::endl;
            json response;
            response["success"] = true;
            response["message"] = "Регистрация успешна! Теперь вы можете войти в систему.";
            return createJsonResponse(response.dump(), 201);
        } else {
            std::cout << "❌ Failed to add user to database: " << email << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка регистрации. Попробуйте еще раз.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleRegister: " << e.what() << std::endl;
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

    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) {
        std::cout << "❌ Failed to create EVP_MD_CTX" << std::endl;
        return "";
    }

    const EVP_MD* md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    // Инициализация
    if (EVP_DigestInit_ex(context, md, nullptr) != 1) {
        std::cout << "❌ EVP_DigestInit_ex failed" << std::endl;
        EVP_MD_CTX_free(context);
        return "";
    }

    // Обновление
    if (EVP_DigestUpdate(context, password.c_str(), password.length()) != 1) {
        std::cout << "❌ EVP_DigestUpdate failed" << std::endl;
        EVP_MD_CTX_free(context);
        return "";
    }

    // Финальное вычисление
    if (EVP_DigestFinal_ex(context, hash, &lengthOfHash) != 1) {
        std::cout << "❌ EVP_DigestFinal_ex failed" << std::endl;
        EVP_MD_CTX_free(context);
        return "";
    }

    EVP_MD_CTX_free(context);

    // Конвертация в hex
    std::stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}


std::string ApiService::handleLogin(const std::string& body) {
    try {
        if (body.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Empty request body";
            return createJsonResponse(errorResponse.dump(), 400);
        }

        json j = json::parse(body);
        
        if (!j.contains("email") || !j.contains("password")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Missing email or password";
            return createJsonResponse(errorResponse.dump(), 400);
        }

        std::string emailOrLogin = j["email"];
        std::string password = j["password"];

        if (emailOrLogin.empty() || password.empty()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Email and password cannot be empty";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::cout << "🔐 Login attempt: " << emailOrLogin << std::endl;
        
        User user = dbService.getUserByEmail(emailOrLogin);
        
        if (user.userId == 0) {
            std::cout << "🔄 Trying login lookup for: " << emailOrLogin << std::endl;
            user = dbService.getUserByLogin(emailOrLogin);
        }
        
        if (user.userId == 0) {
            std::cout << "❌ User not found: " << emailOrLogin << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Пользователь с такими учетными данными не найден";
            return createJsonResponse(errorResponse.dump(), 401);
        }
        
        std::string hashedPassword = hashPassword(password);
        
        if (user.passwordHash != hashedPassword) {
            std::cout << "❌ Password mismatch for user: " << emailOrLogin << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный пароль";
            return createJsonResponse(errorResponse.dump(), 401);
        }
        
        // ГЕНЕРИРУЕМ ТОКЕН
        std::string sessionToken = generateSessionToken();
        std::cout << "✅ Generated session token, length: " << sessionToken.length() << std::endl;
        
        // СОЗДАЕМ СЕССИЮ
        auto now = std::chrono::system_clock::now();
        Session session;
        session.userId = std::to_string(user.userId);
        session.email = user.email;
        session.createdAt = now;
        session.lastActivity = now;
        
        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions[sessionToken] = session;
            std::cout << "💾 Session saved, total sessions: " << sessions.size() << std::endl;
        }
        
        // УСПЕШНЫЙ ОТВЕТ
        json response;
        response["success"] = true;
        response["message"] = "Вход успешно выполнен!";
        response["token"] = sessionToken;
        response["user"] = {
            {"userId", user.userId},
            {"email", user.email},
            {"firstName", user.firstName},
            {"lastName", user.lastName},
            {"login", user.login}
        };
        
        std::cout << "✅ Login successful for: " << emailOrLogin << std::endl;
        
        return createJsonResponse(response.dump());
        
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleLogin: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Ошибка сервера при авторизации";
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

std::string ApiService::handleLogout(const std::string& sessionToken) {
    std::cout << "🚪 Logout request, token: " << (sessionToken.empty() ? "empty" : sessionToken.substr(0, 16) + "...") << std::endl;
    
    if (!sessionToken.empty()) {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        size_t erased = sessions.erase(sessionToken);
        std::cout << "✅ Session removed, total sessions now: " << sessions.size() 
                  << " (erased: " << erased << ")" << std::endl;
    }
    
    json response;
    response["success"] = true;
    response["message"] = "Выход с учётной записи успешно осуществлён";
    return createJsonResponse(response.dump());
}