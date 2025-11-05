#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

std::string ApiService::handleAddTeacher(const std::string& body, const std::string& sessionToken) {
    std::cout << "🔄 Обработка добавления преподавателя..." << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;

    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        Teacher teacher;
        
        // Обязательные поля
        if (!j.contains("last_name") || !j.contains("first_name")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поля 'last_name' и 'first_name' обязательны";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        teacher.lastName = j["last_name"];
        teacher.firstName = j["first_name"];
        teacher.middleName = j.value("middle_name", "");
        teacher.experience = j.value("experience", 0);
        teacher.email = j.value("email", "");
        teacher.phoneNumber = j.value("phone_number", "");
        
        std::cout << "👨‍🏫 Добавление преподавателя: " << teacher.firstName << " " << teacher.lastName << std::endl;
        
        // Обрабатываем специализации
        if (j.contains("specialization") && !j["specialization"].is_null()) {
            std::string specializationStr = j["specialization"];
            std::cout << "🔗 Обработка специализаций: " << specializationStr << std::endl;
            
            // Разделяем строку специализаций по запятой
            size_t start = 0, end = 0;
            while ((end = specializationStr.find(',', start)) != std::string::npos) {
                std::string name = specializationStr.substr(start, end - start);
                // Удаляем пробелы
                name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));
                name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                if (!name.empty()) {
                    Specialization spec;
                    spec.name = name;
                    teacher.specializations.push_back(spec);
                }
                start = end + 1;
            }
            // Добавляем последнюю специализацию
            std::string lastName = specializationStr.substr(start);
            lastName.erase(0, lastName.find_first_not_of(" \t\n\r\f\v"));
            lastName.erase(lastName.find_last_not_of(" \t\n\r\f\v") + 1);
            if (!lastName.empty()) {
                Specialization spec;
                spec.name = lastName;
                teacher.specializations.push_back(spec);
            }
        }
        
        // Добавляем преподавателя в БД
        if (dbService.addTeacher(teacher)) {
            std::cout << "✅ Преподаватель успешно добавлен" << std::endl;
            
            // Формируем успешный ответ
            json response;
            response["success"] = true;
            response["message"] = "Преподаватель успешно добавлен!";
            
            return createJsonResponse(response.dump(), 201);
        } else {
            std::cout << "❌ Ошибка при добавлении преподавателя в БД" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при добавлении преподавателя";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в handleAddTeacher: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleUpdateTeacher(const std::string& body, int teacherId, const std::string& sessionToken) {
    std::cout << "🔄 Обработка обновления преподавателя ID: " << teacherId << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;

    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        
        // Получаем текущие данные преподавателя
        Teacher teacher = dbService.getTeacherById(teacherId);
        
        if (teacher.teacherId == 0) {
            std::cout << "❌ Преподаватель не найден: " << teacherId << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Преподаватель не найден";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        std::cout << "👨‍🏫 Обновление преподавателя ID: " << teacherId << std::endl;
        
        // Обновляем поля
        if (j.contains("last_name")) teacher.lastName = j["last_name"];
        if (j.contains("first_name")) teacher.firstName = j["first_name"];
        if (j.contains("middle_name")) teacher.middleName = j["middle_name"];
        if (j.contains("experience")) teacher.experience = j["experience"];
        if (j.contains("email")) teacher.email = j["email"];
        if (j.contains("phone_number")) teacher.phoneNumber = j["phone_number"];
        
        // Обновляем основные данные преподавателя
        if (dbService.updateTeacher(teacher)) {
            std::cout << "✅ Основные данные преподавателя обновлены" << std::endl;
            
            // ОБНОВЛЯЕМ СПЕЦИАЛИЗАЦИИ - УПРОЩЕННАЯ ЛОГИКА
            if (j.contains("specialization")) {
                std::string specializationStr = j["specialization"];
                std::cout << "🔗 Обновление специализаций: " << specializationStr << std::endl;
                
                // Удаляем все текущие специализации преподавателя
                if (dbService.removeAllTeacherSpecializations(teacherId)) {
                    std::cout << "✅ Старые специализации удалены" << std::endl;
                } else {
                    std::cout << "⚠️ Не удалось удалить старые специализации" << std::endl;
                }
                
                // Добавляем новые специализации, если они есть
                if (!specializationStr.empty()) {
                    // Разделяем строку специализаций по запятой
                    size_t start = 0, end = 0;
                    std::vector<std::string> specNames;
                    
                    while ((end = specializationStr.find(',', start)) != std::string::npos) {
                        std::string name = specializationStr.substr(start, end - start);
                        // Удаляем пробелы
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
                    
                    // Добавляем новые специализации
                    for (const auto& name : specNames) {
                        Specialization spec;
                        spec.specializationCode = teacher.specializationCode;
                        spec.name = name;
                        
                        if (dbService.addSpecialization(spec)) {
                            std::cout << "✅ Добавлена специализация: " << name << " (код: " << teacher.specializationCode << ")" << std::endl;
                        } else {
                            std::cout << "❌ Не удалось добавить специализацию: " << name << std::endl;
                        }
                    }
                }
            }
            
            json response;
            response["success"] = true;
            response["message"] = "Преподаватель успешно обновлен";
            
            return createJsonResponse(response.dump());
        } else {
            std::cout << "❌ Ошибка при обновлении преподавателя" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при обновлении преподавателя";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в handleUpdateTeacher: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleDeleteTeacher(int teacherId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::cout << "👨‍🏫 Deleting teacher ID: " << teacherId << std::endl;
    
    if (dbService.deleteTeacher(teacherId)) {
        std::cout << "✅ Teacher deleted successfully" << std::endl;
        json response;
        response["success"] = true;
        response["message"] = "Teacher deleted successfully";
        return createJsonResponse(response.dump());
    } else {
        std::cout << "❌ Failed to delete teacher" << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Failed to delete teacher";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleAddTeacherSpecialization(const std::string& body, const std::string& sessionToken) {
    std::cout << "🔄 Обработка добавления специализации преподавателю..." << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;

    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }

    try {
        json j = json::parse(body);
        
        if (!j.contains("teacher_id") || !j.contains("specialization_code")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поля 'teacher_id' и 'specialization_code' обязательны";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        int teacherId = j["teacher_id"];
        int specializationCode = j["specialization_code"];
        
        std::cout << "🔗 Добавление специализации " << specializationCode << " преподавателю " << teacherId << std::endl;
        
        if (dbService.addTeacherSpecialization(teacherId, specializationCode)) {
            std::cout << "✅ Специализация успешно добавлена преподавателю" << std::endl;
            
            json response;
            response["success"] = true;
            response["message"] = "Специализация успешно добавлена преподавателю";
            
            return createJsonResponse(response.dump(), 201);
        } else {
            std::cout << "❌ Ошибка при добавлении специализации преподавателю" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при добавлении специализации преподавателю";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в handleAddTeacherSpecialization: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleRemoveTeacherSpecialization(int teacherId, int specializationCode, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::cout << "🔗 Removing specialization " << specializationCode << " from teacher " << teacherId << std::endl;
    
    if (dbService.removeTeacherSpecialization(teacherId, specializationCode)) {
        json response;
        response["success"] = true;
        response["message"] = "Specialization removed from teacher successfully";
        return createJsonResponse(response.dump());
    } else {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Failed to remove specialization from teacher";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleDeleteSpecialization(int specializationCode, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    std::cout << "🗑️ Deleting specialization with code: " << specializationCode << std::endl;
    
    if (dbService.deleteSpecialization(specializationCode)) {
        std::cout << "✅ Specialization deleted successfully" << std::endl;
        json response;
        response["success"] = true;
        response["message"] = "Specialization deleted successfully";
        return createJsonResponse(response.dump());
    } else {
        std::cout << "❌ Failed to delete specialization" << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Failed to delete specialization";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleAddSpecialization(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }

    try {
        json j = json::parse(body);
        
        if (!j.contains("name") || j["name"].is_null()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Field 'name' is required";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string name = j["name"];
        int code = j.value("code", 0);
        
        Specialization spec;
        spec.name = name;
        spec.specializationCode = code;
        
        std::cout << "📚 Adding specialization: " << name << " (code: " << code << ")" << std::endl;
        
        if (dbService.addSpecialization(spec)) {
            json response;
            response["success"] = true;
            response["message"] = "Specialization added successfully";
            response["code"] = code;
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Failed to add specialization";
            return createJsonResponse(errorResponse.dump(), 500);
        }
        
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION in handleAddSpecialization: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid request format";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleAddStudent(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
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
            json response;
            response["success"] = true;
            response["message"] = "Student added successfully";
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Failed to add student";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid request format";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleUpdateStudent(const std::string& body, int studentId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        Student student = dbService.getStudentById(studentId);
        
        if (student.studentCode == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Student not found";
            return createJsonResponse(errorResponse.dump(), 404);
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
            json response;
            response["success"] = true;
            response["message"] = "Student updated successfully";
            return createJsonResponse(response.dump());
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Failed to update student";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid request format";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleDeleteStudent(int studentId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    if (dbService.deleteStudent(studentId)) {
        json response;
        response["success"] = true;
        response["message"] = "Student deleted successfully";
        return createJsonResponse(response.dump());
    } else {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Failed to delete student";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleAddGroup(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        StudentGroup group;
        
        group.name = j["name"];
        group.studentCount = j.value("student_count", 0);
        group.teacherId = j["teacher_id"];
        
        if (dbService.addGroup(group)) {
            json response;
            response["success"] = true;
            response["message"] = "Group added successfully";
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Failed to add group";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid request format";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleUpdateGroup(const std::string& body, int groupId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    try {
        json j = json::parse(body);
        StudentGroup group = dbService.getGroupById(groupId);
        
        if (group.groupId == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Group not found";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        if (j.contains("name")) group.name = j["name"];
        if (j.contains("student_count")) group.studentCount = j["student_count"];
        if (j.contains("teacher_id")) group.teacherId = j["teacher_id"];
        
        if (dbService.updateGroup(group)) {
            json response;
            response["success"] = true;
            response["message"] = "Group updated successfully";
            return createJsonResponse(response.dump());
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Failed to update group";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid request format";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleDeleteGroup(int groupId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
    }
    
    if (dbService.deleteGroup(groupId)) {
        json response;
        response["success"] = true;
        response["message"] = "Group deleted successfully";
        return createJsonResponse(response.dump());
    } else {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Failed to delete group";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleAddPortfolio(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
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
            json response;
            response["success"] = true;
            response["message"] = "Portfolio item added successfully";
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Failed to add portfolio item";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid request format";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleAddEvent(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unauthorized";
        return createJsonResponse(errorResponse.dump(), 401);
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
            json response;
            response["success"] = true;
            response["message"] = "Event added successfully";
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Failed to add event";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Invalid request format";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}
