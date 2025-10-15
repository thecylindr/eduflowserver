#include "api/ApiService.h"
#include "json.hpp"
#include <openssl/rand.h>

using json = nlohmann::json;

std::string ApiService::handleLogin(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string email = j["email"];
        std::string password = j["password"];
        
        User user = dbService.getUserByEmail(email);
        std::string hashedPassword = hashPassword(password);
        
        if (user.userId == 0 || user.passwordHash != hashedPassword) {
            return createJsonResponse("{\"error\": \"Неверный пароль или e-mail.\"}", 401);
        }
        
        // Create session
        std::string sessionToken = generateSessionToken();
        Session session;
        session.userId = std::to_string(user.userId);
        session.email = user.email;
        session.createdAt = std::chrono::system_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions[sessionToken] = session;
        }
        
        json response;
        response["message"] = "Успешная авторизация";
        response["token"] = sessionToken;
        response["user"] = {
            {"userId", user.userId},
            {"email", user.email},
            {"firstName", user.firstName},
            {"lastName", user.lastName}
        };
        
        return createJsonResponse(response.dump());
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Неверный формат запроса.\"}", 400);
    }
}

std::string ApiService::handleForgotPassword(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string email = j["email"];
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            // Don't reveal whether user exists
            return createJsonResponse("{\"message\": \"If the email exists, a reset code has been sent\"}");
        }
        
        // Generate reset token (in a real app, send this via email)
        std::string resetToken = generateSessionToken();
        
        {
            std::lock_guard<std::mutex> lock(passwordResetMutex);
            passwordResetTokens[resetToken] = PasswordResetToken{
                email,
                std::chrono::system_clock::now()
            };
        }
        
        // In a real application, you would send an email here
        // For demo purposes, we'll return the token
        json response;
        response["message"] = "Reset code generated";
        response["resetToken"] = resetToken; // Remove this in production!
        
        return createJsonResponse(response.dump());
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Неверный формат запроса.\"}", 400);
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
                return createJsonResponse("{\"error\": \"Invalid or expired reset token\"}", 400);
            }
            
            // Check if token is expired (1 hour)
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.createdAt);
            if (duration.count() > 60) {
                passwordResetTokens.erase(it);
                return createJsonResponse("{\"error\": \"Reset token expired\"}", 400);
            }
            
            email = it->second.email;
            passwordResetTokens.erase(it);
        }
        
        User user = dbService.getUserByEmail(email);
        if (user.userId == 0) {
            return createJsonResponse("{\"error\": \"User not found\"}", 404);
        }
        
        // Update password
        user.passwordHash = hashPassword(newPassword);
        if (dbService.updateUser(user)) {
            return createJsonResponse("{\"message\": \"Password reset successfully\"}");
        } else {
            return createJsonResponse("{\"error\": \"Password reset failed\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleLogout(const std::string& sessionToken) {
    if (!sessionToken.empty()) {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        sessions.erase(sessionToken);
    }
    
    return createJsonResponse("{\"message\": \"Logged out successfully\"}");
}