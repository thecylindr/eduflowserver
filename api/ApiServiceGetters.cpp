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
    
    // Получаем ВСЕ сессии пользователя из базы данных
    auto userSessions = dbService.getSessionsByUserId(userId);
    json sessionsArray = json::array();
    auto now = std::chrono::system_clock::now();
    
    std::cout << "📊 Загружено сессий из БД: " << userSessions.size() << std::endl;
    
    for (const auto& session : userSessions) {
        if (now > session.expiresAt) continue;
        
        auto age = std::chrono::duration_cast<std::chrono::hours>(now - session.createdAt);
        auto inactive = std::chrono::duration_cast<std::chrono::minutes>(now - session.lastActivity);
        
        // ДЕТАЛЬНОЕ ЛОГИРОВАНИЕ КАЖДОЙ СЕССИИ
        std::cout << "🔍 Сессия DETAIL - Token: " << session.token.substr(0, 16) << "..."
                  << ", OS: '" << session.userOS << "'"
                  << ", IP: '" << session.ipAddress << "'"
                  << ", UserId: " << session.userId 
                  << ", Email: " << session.email << std::endl;
        
        json sessionJson;
        sessionJson["token"] = session.token;
        sessionJson["email"] = session.email;
        sessionJson["userOS"] = session.userOS;
        sessionJson["ipAddress"] = session.ipAddress;
        sessionJson["createdAt"] = std::chrono::duration_cast<std::chrono::seconds>(
            session.createdAt.time_since_epoch()).count();
        sessionJson["lastActivity"] = std::chrono::duration_cast<std::chrono::seconds>(
            session.lastActivity.time_since_epoch()).count();
        sessionJson["ageHours"] = age.count();
        sessionJson["inactiveMinutes"] = inactive.count();
        sessionJson["isCurrent"] = (session.token == sessionToken);
        
        sessionsArray.push_back(sessionJson);
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
    std::cout << "📊 Sessions sent to client: " << sessionsArray.size() << std::endl;
    
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
        
        // Получаем специализации преподавателя
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
        teacherJson["specialization"] = specNames; // Для обратной совместимости
        
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
    std::cout << "📋 Получение списка уникальных специализаций..." << std::endl;
    
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }

    std::lock_guard<std::mutex> lock(dbMutex);
    auto uniqueNames = dbService.getUniqueSpecializationNames();  // Получаем уникальные имена из БД

    json data = json::array();
    for (const auto& name : uniqueNames) {
        json specJson;
        specJson["name"] = name;  // Возвращаем только имя для ComboBox
        data.push_back(specJson);
    }

    json response;
    response["success"] = true;
    response["data"] = data;

    std::cout << "✅ Отправлено уникальных специализаций: " << data.size() << std::endl;
    return createJsonResponse(response.dump());
}

std::string ApiService::getTeacherSpecializationsJson(int teacherId) {
    
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

std::string ApiService::getEventCategoriesJson() {
    std::cout << "📋 Получение списка категорий событий..." << std::endl;
    
    auto categories = dbService.getEventCategories();
    json response;
    response["success"] = true;
    response["data"] = json::array();
    
    for (const auto& category : categories) {
        json categoryJson;
        categoryJson["event_code"] = category.eventCode;
        categoryJson["category"] = category.category;
        
        response["data"].push_back(categoryJson);
    }
    
    return createJsonResponse(response.dump());
}

std::string ApiService::getPortfolioJson(const std::string& sessionToken) {
    std::cout << "📋 Получение списка портфолио..." << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    auto portfolios = dbService.getPortfolios();
    json response;
    response["success"] = true;
    response["data"] = json::array();
    
    for (const auto& portfolio : portfolios) {
        json portfolioJson;
        portfolioJson["portfolio_id"] = portfolio.portfolioId;
        portfolioJson["student_code"] = portfolio.studentCode;
        portfolioJson["student_name"] = portfolio.studentName;
        portfolioJson["date"] = portfolio.date;
        portfolioJson["decree"] = portfolio.decree;
        
        response["data"].push_back(portfolioJson);
    }
    
    std::cout << "✅ Отправлено портфолио: " << response["data"].size() << " записей" << std::endl;
    return createJsonResponse(response.dump());
}

std::string ApiService::handleGetDashboard(const std::string& sessionToken) {
    std::cout << "📊 Получение данных для дашборда..." << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        // Получаем информацию о пользователе
        std::string userId = getUserIdFromSession(sessionToken);
        User user = dbService.getUserById(std::stoi(userId));
        
        if (user.userId == 0) {
            return createJsonResponse("{\"success\": false, \"error\": \"User not found\"}", 404);
        }
        
        // Получаем статистику
        std::lock_guard<std::mutex> lock(dbMutex);
        
        int teachersCount = dbService.getTeachersCount();
        int studentsCount = dbService.getStudentsCount();
        int groupsCount = dbService.getGroupsCount();
        int portfoliosCount = dbService.getPortfoliosCount();
        int eventsCount = dbService.getEventsCount();
        
        // Формируем ответ
        json dashboardData;
        dashboardData["user"] = {
            {"login", user.login},
            {"firstName", user.firstName},
            {"lastName", user.lastName},
            {"email", user.email}
        };
        
        dashboardData["stats"] = {
            {"teachers", teachersCount},
            {"students", studentsCount},
            {"groups", groupsCount},
            {"portfolios", portfoliosCount},
            {"events", eventsCount}
        };
        
        dashboardData["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        json response;
        response["success"] = true;
        response["data"] = dashboardData;
        
        return createJsonResponse(response.dump());
        
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка получения данных дашборда: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Dashboard data error\"}", 500);
    }
}

std::string ApiService::handleGetStudentsByGroup(int groupId) {
    std::cout << "👥 Получение студентов группы ID: " << groupId << std::endl;

    std::lock_guard<std::mutex> lock(dbMutex);
    auto students = dbService.getStudentsByGroup(groupId);
    json j = json::array();

    for (const auto& student : students) {
        json studentJson;
        studentJson["student_code"] = student.studentCode;
        studentJson["last_name"] = student.lastName;
        studentJson["first_name"] = student.firstName;
        studentJson["middle_name"] = student.middleName;
        studentJson["phone_number"] = student.phoneNumber;
        studentJson["email"] = student.email;
        studentJson["group_id"] = student.groupId;
        studentJson["passport_series"] = student.passportSeries;
        studentJson["passport_number"] = student.passportNumber;

        j.push_back(studentJson);
    }

    json response;
    response["success"] = true;
    response["data"] = j;
    return createJsonResponse(response.dump());
}