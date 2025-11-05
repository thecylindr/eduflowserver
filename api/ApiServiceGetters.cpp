// [Замените код в ApiServiceGetters.cpp]

#include "api/ApiService.h"
#include "json.hpp"
#include <mutex>
#include <iostream>

using json = nlohmann::json;

static std::mutex dbMutex;

std::string ApiService::getProfile(const std::string& sessionToken) {
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
    
    std::lock_guard<std::mutex> lock(dbMutex);
    User user = dbService.getUserById(std::stoi(userId));
    if (user.userId == 0) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "User not found";
        return createJsonResponse(errorResponse.dump(), 404);
    }
    
    // Логируем данные для отладки
    std::cout << "📊 Данные пользователя из БД:" << std::endl;
    std::cout << "   ID: " << user.userId << std::endl;
    std::cout << "   Логин: '" << user.login << "'" << std::endl;
    std::cout << "   Email: '" << user.email << "'" << std::endl;
    std::cout << "   Имя: '" << user.firstName << "'" << std::endl;
    std::cout << "   Фамилия: '" << user.lastName << "'" << std::endl;
    std::cout << "   Отчество: '" << user.middleName << "'" << std::endl;
    std::cout << "   Телефон: '" << user.phoneNumber << "'" << std::endl;
    
    // Получаем сессии
    std::lock_guard<std::mutex> sessionsLock(sessionsMutex);
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
    
    // Формируем ответ с реальными данными
    json userJson;
    userJson["userId"] = user.userId;
    userJson["login"] = user.login;
    userJson["email"] = user.email;
    userJson["firstName"] = user.firstName;
    userJson["lastName"] = user.lastName;
    userJson["middleName"] = user.middleName;
    userJson["phoneNumber"] = user.phoneNumber;
    userJson["sessions"] = sessionsArray;
    
    json response;
    response["success"] = true;
    response["data"] = userJson;
    
    std::cout << "✅ Profile data sent for user: " << user.login << std::endl;
    
    return createJsonResponse(response.dump());
}

std::string ApiService::getTeachersJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }

    std::lock_guard<std::mutex> lock(dbMutex);
    auto teachers = dbService.getTeachers();
    json teachersArray = json::array();
    
    for (auto& teacher : teachers) {
        json teacherJson;
        teacherJson["teacher_id"] = teacher.teacherId;
        teacherJson["last_name"] = teacher.lastName;
        teacherJson["first_name"] = teacher.firstName;
        teacherJson["middle_name"] = teacher.middleName;
        teacherJson["experience"] = teacher.experience;
        teacherJson["email"] = teacher.email;
        teacherJson["phone_number"] = teacher.phoneNumber;
        
        auto specializations = dbService.getTeacherSpecializations(teacher.teacherId);
        json specArray = json::array();
        
        std::string specNames;
        for (const auto& spec : specializations) {
            json specJson;
            specJson["code"] = spec.specializationCode;
            specJson["name"] = spec.name;
            specArray.push_back(specJson);
            
            if (!specNames.empty()) {
                specNames += ", ";
            }
            specNames += spec.name;
        }
        
        teacherJson["specializations"] = specArray;
        teacherJson["specialization"] = specNames;
        
        teachersArray.push_back(teacherJson);
    }
    
    json response;
    response["success"] = true;
    response["data"] = teachersArray;
    
    return createJsonResponse(response.dump());
}

std::string ApiService::getStudentsJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::lock_guard<std::mutex> lock(dbMutex);
    auto students = dbService.getStudents();
    json j = json::array();
    
    for (const auto& student : students) {
        json studentJson;
        studentJson["studentCode"] = student.studentCode;
        studentJson["lastName"] = student.lastName;
        studentJson["firstName"] = student.firstName;
        studentJson["middleName"] = student.middleName;
        studentJson["phoneNumber"] = student.phoneNumber;
        studentJson["email"] = student.email;
        studentJson["groupId"] = student.groupId;
        studentJson["passportSeries"] = student.passportSeries;
        studentJson["passportNumber"] = student.passportNumber;
        
        j.push_back(studentJson);
    }
    
    json response;
    response["success"] = true;
    response["data"] = j;
    return createJsonResponse(response.dump());
}

std::string ApiService::getGroupsJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::lock_guard<std::mutex> lock(dbMutex);
    auto groups = dbService.getGroups();
    json j = json::array();
    
    for (const auto& group : groups) {
        json groupJson;
        groupJson["groupId"] = group.groupId;
        groupJson["name"] = group.name;
        groupJson["studentCount"] = group.studentCount;
        groupJson["teacherId"] = group.teacherId;
        
        j.push_back(groupJson);
    }
    
    json response;
    response["success"] = true;
    response["data"] = j;
    return createJsonResponse(response.dump());
}

std::string ApiService::getSpecializationsJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::lock_guard<std::mutex> lock(dbMutex);
    auto specializations = dbService.getSpecializations();
    json j = json::array();
    
    for (const auto& spec : specializations) {
        json specJson;
        specJson["code"] = spec.specializationCode;
        specJson["name"] = spec.name;
        j.push_back(specJson);
    }
    
    json response;
    response["success"] = true;
    response["data"] = j;
    return createJsonResponse(response.dump());
}

std::string ApiService::getTeacherSpecializationsJson(int teacherId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::lock_guard<std::mutex> lock(dbMutex);
    auto specializations = dbService.getTeacherSpecializations(teacherId);
    json j = json::array();
    
    for (const auto& spec : specializations) {
        json specJson;
        specJson["code"] = spec.specializationCode;
        specJson["name"] = spec.name;
        j.push_back(specJson);
    }
    
    json response;
    response["success"] = true;
    response["data"] = j;
    return createJsonResponse(response.dump());
}

std::string ApiService::getPortfolioJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::lock_guard<std::mutex> lock(dbMutex);
    auto portfolios = dbService.getPortfolios();
    json j = json::array();
    
    for (const auto& portfolio : portfolios) {
        json portfolioJson;
        portfolioJson["portfolioId"] = portfolio.portfolioId;
        portfolioJson["studentCode"] = portfolio.studentCode;
        portfolioJson["measureCode"] = portfolio.measureCode;
        portfolioJson["date"] = portfolio.date;
        portfolioJson["passportSeries"] = portfolio.passportSeries;
        portfolioJson["passportNumber"] = portfolio.passportNumber;
        j.push_back(portfolioJson);
    }
    
    json response;
    response["success"] = true;
    response["data"] = j;
    return createJsonResponse(response.dump());
}

std::string ApiService::getEventsJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::lock_guard<std::mutex> lock(dbMutex);
    auto events = dbService.getEvents();
    json j = json::array();
    
    for (const auto& event : events) {
        json eventJson;
        eventJson["eventId"] = event.eventId;
        eventJson["eventCategory"] = event.eventCategory;
        eventJson["eventType"] = event.eventType;
        eventJson["startDate"] = event.startDate;
        eventJson["endDate"] = event.endDate;
        eventJson["location"] = event.location;
        eventJson["lore"] = event.lore;
        j.push_back(eventJson);
    }
    
    json response;
    response["success"] = true;
    response["data"] = j;
    return createJsonResponse(response.dump());
}