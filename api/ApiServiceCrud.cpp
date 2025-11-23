#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

std::string ApiService::handleAddTeacher(const std::string& body) {
    std::cout << "🔄 Обработка добавления преподавателя..." << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;
    
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

std::string ApiService::handleUpdateTeacher(const std::string& body, int teacherId) {
    std::cout << "🔄 Обработка обновления преподавателя ID: " << teacherId << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;
    
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
        
        if (dbService.updateTeacher(teacher)) {
            std::cout << "✅ Основные данные преподавателя обновлены" << std::endl;
            
            if (j.contains("specialization")) {
                std::string specializationStr = j["specialization"];
                std::cout << "🔗 Обновление специализаций: " << specializationStr << std::endl;
                
                // Получаем текущий код специализации преподавателя
                int currentSpecCode = teacher.specializationCode;
                std::cout << "🔑 Текущий код специализации: " << currentSpecCode << std::endl;
                
                if (currentSpecCode > 0) {
                    // УДАЛЯЕМ ВСЕ СТАРЫЕ СПЕЦИАЛИЗАЦИИ ЭТОГО ПРЕПОДАВАТЕЛЯ
                    if (dbService.removeAllTeacherSpecializations(teacherId)) {
                        std::cout << "✅ Старые специализации удалены" << std::endl;
                    } else {
                        std::cout << "⚠️ Не удалось удалить старые специализации" << std::endl;
                    }
                    
                    // ДОБАВЛЯЕМ НОВЫЕ СПЕЦИАЛИЗАЦИИ (только если есть новые)
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
                        
                        for (const auto& name : specNames) {
                            Specialization spec;
                            spec.specializationCode = currentSpecCode;
                            spec.name = name;
                            
                            if (dbService.addSpecialization(spec)) {
                                std::cout << "✅ Добавлена специализация: " << name << " (код: " << currentSpecCode << ")" << std::endl;
                            } else {
                                std::cout << "❌ Не удалось добавить специализацию: " << name << std::endl;
                            }
                        }
                    }
                } else {
                    std::cout << "⚠️ У преподавателя нет кода специализации, пропускаем обновление специализаций" << std::endl;
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

std::string ApiService::handleDeleteTeacher(int teacherId) {
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

std::string ApiService::handleAddTeacherSpecialization(const std::string& body) {
    std::cout << "🔄 Обработка добавления специализации преподавателю..." << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;

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

std::string ApiService::handleRemoveTeacherSpecialization(int teacherId, int specializationCode) {
    
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

std::string ApiService::handleDeleteSpecialization(int specializationCode) {
    
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

std::string ApiService::handleAddSpecialization(const std::string& body) {

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

std::string ApiService::handleAddStudent(const std::string& body) {
    std::cout << "➕ Добавление студента..." << std::endl;
    
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
            // ОБНОВЛЯЕМ СЧЕТЧИК ГРУППЫ
            if (student.groupId > 0) {
                dbService.updateGroupStudentCount(student.groupId, 1);
            }
            
            json response;
            response["success"] = true;
            response["message"] = "Студент успешно добавлен";
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка добавления студента";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка добавления студента: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleUpdateStudent(const std::string& body, int studentId) {
    std::cout << "🔄 Обновление студента ID: " << studentId << std::endl;
    
    try {
        json j = json::parse(body);
        Student oldStudent = dbService.getStudentById(studentId);
        
        if (oldStudent.studentCode == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Студент не найден";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        Student newStudent = oldStudent;
        
        // Обновляем поля
        if (j.contains("last_name")) newStudent.lastName = j["last_name"];
        if (j.contains("first_name")) newStudent.firstName = j["first_name"];
        if (j.contains("middle_name")) newStudent.middleName = j["middle_name"];
        if (j.contains("phone_number")) newStudent.phoneNumber = j["phone_number"];
        if (j.contains("email")) newStudent.email = j["email"];
        if (j.contains("group_id")) newStudent.groupId = j["group_id"];
        if (j.contains("passport_series")) newStudent.passportSeries = j["passport_series"];
        if (j.contains("passport_number")) newStudent.passportNumber = j["passport_number"];
        
        // ОБРАБАТЫВАЕМ ИЗМЕНЕНИЕ ГРУППЫ
        if (oldStudent.groupId != newStudent.groupId) {
            std::cout << "🔄 Изменение группы студента: " 
                      << oldStudent.groupId << " -> " << newStudent.groupId << std::endl;
            
            // Уменьшаем счетчик старой группы
            if (oldStudent.groupId > 0) {
                dbService.updateGroupStudentCount(oldStudent.groupId, -1);
            }
            
            // Увеличиваем счетчик новой группы
            if (newStudent.groupId > 0) {
                dbService.updateGroupStudentCount(newStudent.groupId, 1);
            }
        }
        
        if (dbService.updateStudent(newStudent)) {
            json response;
            response["success"] = true;
            response["message"] = "Студент успешно обновлен";
            return createJsonResponse(response.dump());
        } else {
            // ЕСЛИ ОШИБКА - ВОССТАНАВЛИВАЕМ СЧЕТЧИКИ
            if (oldStudent.groupId != newStudent.groupId) {
                if (oldStudent.groupId > 0) {
                    dbService.updateGroupStudentCount(oldStudent.groupId, 1);
                }
                if (newStudent.groupId > 0) {
                    dbService.updateGroupStudentCount(newStudent.groupId, -1);
                }
            }
            
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка обновления студента";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка обновления студента: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleDeleteStudent(int studentId) {
    std::cout << "🗑️ Удаление студента ID: " << studentId << std::endl;
    
    // Получаем данные студента перед удалением
    Student student = dbService.getStudentById(studentId);
    
    if (dbService.deleteStudent(studentId)) {
        // УМЕНЬШАЕМ СЧЕТЧИК ГРУППЫ
        if (student.groupId > 0) {
            dbService.updateGroupStudentCount(student.groupId, -1);
        }
        
        json response;
        response["success"] = true;
        response["message"] = "Студент успешно удален";
        return createJsonResponse(response.dump());
    } else {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Ошибка удаления студента";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleAddGroup(const std::string& body) {
    
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

std::string ApiService::handleUpdateGroup(const std::string& body, int groupId) {
    
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

std::string ApiService::handleDeleteGroup(int groupId) {
    
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

std::string ApiService::getEventsJson(const std::string& sessionToken) {
    std::cout << "📅 Получение списка событий..." << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    auto events = dbService.getEvents();
    json response;
    response["success"] = true;
    response["data"] = json::array();
    
    for (const auto& event : events) {
        json eventJson;
        eventJson["id"] = event.eventId;
        eventJson["event_id"] = event.measureCode;
        eventJson["event_type"] = event.eventType;
        eventJson["category"] = event.category;
        eventJson["start_date"] = event.startDate;
        eventJson["end_date"] = event.endDate;
        eventJson["location"] = event.location;
        eventJson["lore"] = event.lore;        
        
        response["data"].push_back(eventJson);
    }
    
    return createJsonResponse(response.dump());
}