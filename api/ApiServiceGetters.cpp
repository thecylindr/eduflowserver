#include "api/ApiService.h"
#include "json.hpp"

using json = nlohmann::json;

std::string ApiService::getProfile(const std::string& sessionToken) {
    std::string userId = getUserIdFromSession(sessionToken);
    if (userId.empty()) {
        return createJsonResponse("{\"error\": \"Invalid session\"}", 401);
    }
    
    User user = dbService.getUserById(std::stoi(userId));
    if (user.userId == 0) {
        return createJsonResponse("{\"error\": \"User not found\"}", 404);
    }
    
    json userJson;
    userJson["userId"] = user.userId;
    userJson["email"] = user.email;
    userJson["firstName"] = user.firstName;
    userJson["lastName"] = user.lastName;
    userJson["middleName"] = user.middleName;
    userJson["phoneNumber"] = user.phoneNumber;
    
    return createJsonResponse(userJson.dump());
}

std::string ApiService::getTeachersJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    auto teachers = dbService.getTeachers();
    json j = json::array();
    
    for (const auto& teacher : teachers) {
        json teacherJson;
        teacherJson["teacherId"] = teacher.teacherId;
        teacherJson["lastName"] = teacher.lastName;
        teacherJson["firstName"] = teacher.firstName;
        teacherJson["middleName"] = teacher.middleName;
        teacherJson["experience"] = teacher.experience;
        teacherJson["specialization"] = teacher.specialization;
        teacherJson["email"] = teacher.email;
        teacherJson["phoneNumber"] = teacher.phoneNumber;
        
        j.push_back(teacherJson);
    }
    
    return createJsonResponse(j.dump(4));
}

std::string ApiService::getStudentsJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
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
    
    return createJsonResponse(j.dump(4));
}

std::string ApiService::getGroupsJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
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
    
    return createJsonResponse(j.dump(4));
}

std::string ApiService::getPortfolioJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
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
    
    return createJsonResponse(j.dump(4));
}

std::string ApiService::getEventsJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
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
    
    return createJsonResponse(j.dump(4));
}

std::string ApiService::getSpecializationsJson(const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    auto specializations = dbService.getSpecializations();
    json j = json::array();
    
    for (const auto& specialization : specializations) {
        j.push_back(specialization);
    }
    
    return createJsonResponse(j.dump(4));
}

std::string ApiService::handleStatus() {
    json response;
    response["status"] = "running";
    response["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::lock_guard<std::mutex> lock(sessionsMutex);
    response["activeSessions"] = sessions.size();
    response["databaseConnected"] = dbService.testConnection();
    response["apiConfig"] = {
        {"port", apiConfig.port},
        {"host", apiConfig.host},
        {"maxConnections", apiConfig.maxConnections},
        {"sessionTimeoutHours", apiConfig.sessionTimeoutHours}
    };
    
    return createJsonResponse(response.dump());
}