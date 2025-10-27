#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

std::string ApiService::handleAddTeacher(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        Teacher teacher;
        
        teacher.lastName = j["last_name"];
        teacher.firstName = j["first_name"];
        teacher.middleName = j.value("middle_name", "");
        teacher.experience = j.value("experience", 0);
        teacher.email = j.value("email", "");
        teacher.phoneNumber = j.value("phone_number", "");
        
        std::cout << "👨‍🏫 Adding teacher: " << teacher.firstName << " " << teacher.lastName << std::endl;
        
        // Обрабатываем специализации из массива
        std::vector<std::string> specNames;
        if (j.contains("specialization") && !j["specialization"].is_null()) {
            std::string specializationStr = j["specialization"];
            std::cout << "🔗 Processing specializations: " << specializationStr << std::endl;
            
            // Разделяем строку специализаций по запятой
            size_t start = 0, end = 0;
            while ((end = specializationStr.find(',', start)) != std::string::npos) {
                std::string name = specializationStr.substr(start, end - start);
                name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));
                name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                if (!name.empty()) {
                    specNames.push_back(name);
                }
                start = end + 1;
            }
            // Добавляем последнюю специализацию
            std::string lastName = specializationStr.substr(start);
            lastName.erase(0, lastName.find_first_not_of(" \t\n\r\f\v"));
            lastName.erase(lastName.find_last_not_of(" \t\n\r\f\v") + 1);
            if (!lastName.empty()) {
                specNames.push_back(lastName);
            }
            
            // Создаем объекты Specialization
            for (const auto& name : specNames) {
                Specialization spec;
                spec.name = name;
                teacher.specializations.push_back(spec);
            }
        }
        
        // Добавляем преподавателя в БД
        if (dbService.addTeacher(teacher)) {
            std::cout << "✅ Teacher added successfully with specializations" << std::endl;
            
            // Получаем обновленный список преподавателей
            auto teachers = dbService.getTeachers();
            json teachersArray = json::array();
            
            for (const auto& t : teachers) {
                json teacherJson;
                teacherJson["teacher_id"] = t.teacherId;
                teacherJson["last_name"] = t.lastName;
                teacherJson["first_name"] = t.firstName;
                teacherJson["middle_name"] = t.middleName;
                teacherJson["experience"] = t.experience;
                teacherJson["email"] = t.email;
                teacherJson["phone_number"] = t.phoneNumber;
                
                // Добавляем специализации преподавателя
                auto specializations = dbService.getTeacherSpecializations(t.teacherId);
                json specArray = json::array();
                for (const auto& spec : specializations) {
                    json specJson;
                    specJson["code"] = spec.specializationCode;
                    specJson["name"] = spec.name;
                    specArray.push_back(specJson);
                }
                teacherJson["specializations"] = specArray;
                
                teachersArray.push_back(teacherJson);
            }
            
            // Формируем полный ответ
            json response;
            response["success"] = true;
            response["message"] = "Преподаватель успешно добавлен";
            response["teacher_id"] = 4; // Временное значение
            response["data"] = teachersArray; // Отправляем весь обновленный список
            
            std::string responseStr = response.dump();
            std::cout << "📤 Sending updated teachers list, count: " << teachersArray.size() << std::endl;
            
            return createJsonResponse(responseStr, 201);
        } else {
            std::cout << "❌ Failed to add teacher" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при добавлении преподавателя";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleAddTeacher: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleUpdateTeacher(const std::string& body, int teacherId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        Teacher teacher = dbService.getTeacherById(teacherId);
        
        if (teacher.teacherId == 0) {
            std::cout << "❌ Teacher not found: " << teacherId << std::endl;
            return createJsonResponse("{\"error\": \"Teacher not found\"}", 404);
        }
        
        std::cout << "👨‍🏫 Updating teacher ID: " << teacherId << std::endl;
        
        if (j.contains("last_name")) teacher.lastName = j["last_name"];
        if (j.contains("first_name")) teacher.firstName = j["first_name"];
        if (j.contains("middle_name")) teacher.middleName = j["middle_name"];
        if (j.contains("experience")) teacher.experience = j["experience"];
        if (j.contains("email")) teacher.email = j["email"];
        if (j.contains("phone_number")) teacher.phoneNumber = j["phone_number"];
        
        if (dbService.updateTeacher(teacher)) {
            std::cout << "✅ Teacher updated successfully" << std::endl;
            return createJsonResponse("{\"message\": \"Teacher updated successfully\"}");
        } else {
            std::cout << "❌ Failed to update teacher" << std::endl;
            return createJsonResponse("{\"error\": \"Failed to update teacher\"}", 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleUpdateTeacher: " << e.what() << std::endl;
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleDeleteTeacher(int teacherId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    std::cout << "👨‍🏫 Deleting teacher ID: " << teacherId << std::endl;
    
    if (dbService.deleteTeacher(teacherId)) {
        std::cout << "✅ Teacher deleted successfully" << std::endl;
        return createJsonResponse("{\"message\": \"Teacher deleted successfully\"}");
    } else {
        std::cout << "❌ Failed to delete teacher" << std::endl;
        return createJsonResponse("{\"error\": \"Failed to delete teacher\"}", 500);
    }
}

std::string ApiService::handleAddTeacherSpecialization(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }

    try {
        json j = json::parse(body);
        
        if (!j.contains("teacher_id") || !j.contains("specialization_code")) {
            return createJsonResponse("{\"error\": \"Fields 'teacher_id' and 'specialization_code' are required\"}", 400);
        }
        
        int teacherId = j["teacher_id"];
        int specializationCode = j["specialization_code"];
        
        std::cout << "🔗 Adding specialization " << specializationCode << " to teacher " << teacherId << std::endl;
        
        if (dbService.addTeacherSpecialization(teacherId, specializationCode)) {
            return createJsonResponse("{\"message\": \"Specialization added to teacher successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Failed to add specialization to teacher\"}", 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleAddTeacherSpecialization: " << e.what() << std::endl;
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleRemoveTeacherSpecialization(int teacherId, int specializationCode, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    std::cout << "🔗 Removing specialization " << specializationCode << " from teacher " << teacherId << std::endl;
    
    if (dbService.removeTeacherSpecialization(teacherId, specializationCode)) {
        return createJsonResponse("{\"message\": \"Specialization removed from teacher successfully\"}");
    } else {
        return createJsonResponse("{\"error\": \"Failed to remove specialization from teacher\"}", 500);
    }
}

std::string ApiService::handleDeleteSpecialization(int specializationCode, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    std::cout << "🗑️ Deleting specialization with code: " << specializationCode << std::endl;
    
    if (dbService.deleteSpecialization(specializationCode)) {
        std::cout << "✅ Specialization deleted successfully" << std::endl;
        return createJsonResponse("{\"message\": \"Specialization deleted successfully\"}");
    } else {
        std::cout << "❌ Failed to delete specialization" << std::endl;
        return createJsonResponse("{\"error\": \"Failed to delete specialization\"}", 500);
    }
}

std::string ApiService::handleAddSpecialization(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }

    try {
        json j = json::parse(body);
        
        if (!j.contains("name") || j["name"].is_null()) {
            return createJsonResponse("{\"error\": \"Field 'name' is required\"}", 400);
        }
        
        std::string name = j["name"];
        int code = j.value("code", 0);
        
        Specialization spec;
        spec.name = name;
        spec.specializationCode = code;
        
        std::cout << "📚 Adding specialization: " << name << " (code: " << code << ")" << std::endl;
        
        if (dbService.addSpecialization(spec)) {
            return createJsonResponse("{\"message\": \"Specialization added successfully\", \"code\": " + std::to_string(code) + "}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Failed to add specialization\"}", 500);
        }
        
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleAddSpecialization: " << e.what() << std::endl;
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleAddStudent(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        Student student;
        
        student.lastName = j["last_name"];
        student.firstName = j["first_name"];
        student.middleName = j.value("middle_name", "");
        student.phoneNumber = j.value("phone_number", "");
        student.email = j.value("email", "");
        student.groupId = j["group_id"];
        student.passportSeries = j["passport_series"];
        student.passportNumber = j["passport_number"];
        
        if (dbService.addStudent(student)) {
            return createJsonResponse("{\"message\": \"Student added successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Failed to add student\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleUpdateStudent(const std::string& body, int studentId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        Student student = dbService.getStudentById(studentId);
        
        if (student.studentCode == 0) {
            return createJsonResponse("{\"error\": \"Student not found\"}", 404);
        }
        
        if (j.contains("last_name")) student.lastName = j["last_name"];
        if (j.contains("first_name")) student.firstName = j["first_name"];
        if (j.contains("middle_name")) student.middleName = j["middle_name"];
        if (j.contains("phone_number")) student.phoneNumber = j["phone_number"];
        if (j.contains("email")) student.email = j["email"];
        if (j.contains("group_id")) student.groupId = j["group_id"];
        if (j.contains("passport_series")) student.passportSeries = j["passport_series"];
        if (j.contains("passport_number")) student.passportNumber = j["passport_number"];
        
        if (dbService.updateStudent(student)) {
            return createJsonResponse("{\"message\": \"Student updated successfully\"}");
        } else {
            return createJsonResponse("{\"error\": \"Failed to update student\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleDeleteStudent(int studentId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    if (dbService.deleteStudent(studentId)) {
        return createJsonResponse("{\"message\": \"Student deleted successfully\"}");
    } else {
        return createJsonResponse("{\"error\": \"Failed to delete student\"}", 500);
    }
}

std::string ApiService::handleAddGroup(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        StudentGroup group;
        
        group.name = j["name"];
        group.studentCount = j.value("student_count", 0);
        group.teacherId = j["teacher_id"];
        
        if (dbService.addGroup(group)) {
            return createJsonResponse("{\"message\": \"Group added successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Failed to add group\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleUpdateGroup(const std::string& body, int groupId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        StudentGroup group = dbService.getGroupById(groupId);
        
        if (group.groupId == 0) {
            return createJsonResponse("{\"error\": \"Group not found\"}", 404);
        }
        
        if (j.contains("name")) group.name = j["name"];
        if (j.contains("student_count")) group.studentCount = j["student_count"];
        if (j.contains("teacher_id")) group.teacherId = j["teacher_id"];
        
        if (dbService.updateGroup(group)) {
            return createJsonResponse("{\"message\": \"Group updated successfully\"}");
        } else {
            return createJsonResponse("{\"error\": \"Failed to update group\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleDeleteGroup(int groupId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    if (dbService.deleteGroup(groupId)) {
        return createJsonResponse("{\"message\": \"Group deleted successfully\"}");
    } else {
        return createJsonResponse("{\"error\": \"Failed to delete group\"}", 500);
    }
}

std::string ApiService::handleAddPortfolio(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        StudentPortfolio portfolio;
        
        portfolio.studentCode = j["student_code"];
        portfolio.measureCode = j["measure_code"];
        portfolio.date = j["date"];
        portfolio.passportSeries = j.value("passport_series", "");
        portfolio.passportNumber = j.value("passport_number", "");
        
        if (dbService.addPortfolio(portfolio)) {
            return createJsonResponse("{\"message\": \"Portfolio item added successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Failed to add portfolio item\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleAddEvent(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        Event event;
        
        event.eventId = j["event_id"];
        event.eventCategory = j["event_category"];
        event.eventType = j["event_type"];
        event.startDate = j["start_date"];
        event.endDate = j["end_date"];
        event.location = j.value("location", "");
        event.lore = j.value("lore", "");
        
        if (dbService.addEvent(event)) {
            return createJsonResponse("{\"message\": \"Event added successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Failed to add event\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleUpdateProfile(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        std::string userId = getUserIdFromSession(sessionToken);
        if (userId.empty()) {
            return createJsonResponse("{\"error\": \"Invalid session\"}", 401);
        }
        
        User user = dbService.getUserById(std::stoi(userId));
        if (user.userId == 0) {
            return createJsonResponse("{\"error\": \"User not found\"}", 404);
        }
        
        if (j.contains("email")) user.email = j["email"];
        if (j.contains("phoneNumber")) user.phoneNumber = j["phoneNumber"];
        if (j.contains("lastName")) user.lastName = j["lastName"];
        if (j.contains("firstName")) user.firstName = j["firstName"];
        if (j.contains("middleName")) user.middleName = j["middleName"];
        
        if (j.contains("password")) {
            user.passwordHash = hashPassword(j["password"]);
        }
        
        if (dbService.updateUser(user)) {
            return createJsonResponse("{\"message\": \"Profile updated successfully\"}");
        } else {
            return createJsonResponse("{\"error\": \"Failed to update profile\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}