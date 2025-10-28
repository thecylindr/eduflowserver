#include "api/ApiService.h"
#include "json.hpp"
#include <mutex>

using json = nlohmann::json;

static std::mutex dbMutex;

// Получение списка специализаций с кодами
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
    
    for (const auto& specialization : specializations) {
        json specJson;
        specJson["code"] = specialization.specializationCode;
        specJson["name"] = specialization.name;
        j.push_back(specJson);
    }

    json response;
    response["success"] = true;
    response["data"] = j;
    return createJsonResponse(response.dump());
}

// Получение преподавателей со специализациями
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
        
        // Получаем специализации преподавателя
        auto specializations = dbService.getTeacherSpecializations(teacher.teacherId);
        json specArray = json::array();
        
        // Формируем строку специализаций для обратной совместимости
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
        teacherJson["specialization"] = specNames; // Для обратной совместимости
        
        teachersArray.push_back(teacherJson);
    }
    
    // Единый формат ответа
    json response;
    response["success"] = true;
    response["data"] = teachersArray;
    
    return createJsonResponse(response.dump());
}

// Получение специализаций конкретного преподавателя
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

// Остальные методы без изменений...
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
    
    json userJson;
    userJson["userId"] = user.userId;
    userJson["email"] = user.email;
    userJson["firstName"] = user.firstName;
    userJson["lastName"] = user.lastName;
    userJson["middleName"] = user.middleName;
    userJson["phoneNumber"] = user.phoneNumber;
    
    json response;
    response["success"] = true;
    response["data"] = userJson;
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

std::string ApiService::handleStatus() {
    json response;
    response["status"] = "running";
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::lock_guard<std::mutex> lock(sessionsMutex);
    response["activeSessions"] = sessions.size();
    
    {
        std::lock_guard<std::mutex> dbLock(dbMutex);
        response["databaseConnected"] = dbService.testConnection();
    }
    
    response["apiConfig"] = {
        {"port", apiConfig.port},
        {"host", apiConfig.host},
        {"maxConnections", apiConfig.maxConnections},
        {"sessionTimeoutHours", apiConfig.sessionTimeoutHours}
    };
    
    response["success"] = true;
    return createJsonResponse(response.dump());
}