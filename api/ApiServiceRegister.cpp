#include "api/ApiService.h"
#include "json.hpp"
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

using json = nlohmann::json;

std::string ApiService::handleRegister(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string email = j["email"];
        std::string password = j["password"];
        std::string firstName = j["firstName"];
        std::string lastName = j["lastName"];
        std::string middleName = j.value("middleName", "");
        std::string phoneNumber = j.value("phoneNumber", "");
        
        // Check if user already exists
        if (dbService.getUserByEmail(email).userId != 0) {
            return createJsonResponse("{\"error\": \"User already exists\"}", 400);
        }
        
        // Create user
        User user;
        user.email = email;
        user.passwordHash = hashPassword(password);
        user.firstName = firstName;
        user.lastName = lastName;
        user.middleName = middleName;
        user.phoneNumber = phoneNumber;
        
        if (dbService.addUser(user)) {
            return createJsonResponse("{\"message\": \"User registered successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Registration failed\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
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