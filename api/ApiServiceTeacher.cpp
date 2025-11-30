#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

std::string ApiService::handleAddTeacher(const std::string& body) {
    try {
        json j = json::parse(body);
        Teacher teacher;
        
        if (!j.contains("last_name") || !j.contains("first_name")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поля фамилия и имя обязательны.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        teacher.lastName = j["last_name"];
        teacher.firstName = j["first_name"];
        teacher.middleName = j.value("middle_name", "");
        teacher.experience = j.value("experience", 0);
        teacher.email = j.value("email", "");
        teacher.phoneNumber = j.value("phone_number", "");
        
        if (!teacher.phoneNumber.empty() && !isValidPhoneNumber(teacher.phoneNumber)) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Номер телефона должен содержать ровно 11 цифр.";
            return createJsonResponse(errorResponse.dump(), 400);
        }

        size_t atPos = teacher.email.find('@');
        if (!teacher.email.empty() && atPos == std::string::npos) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный формат почты.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (j.contains("specialization") && !j["specialization"].is_null()) {
            std::string specializationStr = j["specialization"];
            
            size_t start = 0, end = 0;
            while ((end = specializationStr.find(',', start)) != std::string::npos) {
                std::string name = specializationStr.substr(start, end - start);
                name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));
                name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                if (!name.empty()) {
                    Specialization spec;
                    spec.name = name;
                    teacher.specializations.push_back(spec);
                }
                start = end + 1;
            }
            std::string lastName = specializationStr.substr(start);
            lastName.erase(0, lastName.find_first_not_of(" \t\n\r\f\v"));
            lastName.erase(lastName.find_last_not_of(" \t\n\r\f\v") + 1);
            if (!lastName.empty()) {
                Specialization spec;
                spec.name = lastName;
                teacher.specializations.push_back(spec);
            }
        }
        
        if (dbService.addTeacher(teacher)) {
            json response;
            response["success"] = true;
            response["message"] = "Преподаватель успешно добавлен!";
            
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при добавлении преподавателя.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleUpdateTeacher(const std::string& body, int teacherId) {
    try {
        json j = json::parse(body);
        
        Teacher teacher = dbService.getTeacherById(teacherId);
        
        if (teacher.teacherId == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Преподаватель не найден.";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        if (j.contains("last_name")) teacher.lastName = j["last_name"];
        if (j.contains("first_name")) teacher.firstName = j["first_name"];
        if (j.contains("middle_name")) teacher.middleName = j["middle_name"];
        if (j.contains("experience")) teacher.experience = j["experience"];
        if (j.contains("email")) teacher.email = j["email"];
        if (j.contains("phone_number")) teacher.phoneNumber = j["phone_number"];
        
        if (!teacher.phoneNumber.empty() && !isValidPhoneNumber(teacher.phoneNumber)) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Номер телефона должен содержать ровно 11 цифр.";
            return createJsonResponse(errorResponse.dump(), 400);
        }

        size_t atPos = teacher.email.find('@');
        if (!teacher.email.empty() && atPos == std::string::npos) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный формат почты.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (dbService.updateTeacher(teacher)) {
            if (j.contains("specialization")) {
                std::string specializationStr = j["specialization"];
                
                int currentSpecCode = teacher.specializationCode;
                
                if (currentSpecCode > 0) {
                    dbService.removeAllTeacherSpecializations(teacherId);
                    
                    if (!specializationStr.empty()) {
                        size_t start = 0, end = 0;
                        std::vector<std::string> specNames;
                        
                        while ((end = specializationStr.find(',', start)) != std::string::npos) {
                            std::string name = specializationStr.substr(start, end - start);
                            name.erase(0, name.find_first_not_of(" \t\n\r\f\v"));
                            name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);
                            if (!name.empty()) {
                                specNames.push_back(name);
                            }
                            start = end + 1;
                        }
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
                            dbService.addSpecialization(spec);
                        }
                    }
                }
            }
            
            json response;
            response["success"] = true;
            response["message"] = "Преподаватель успешно обновлен.";
            
            return createJsonResponse(response.dump());
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при обновлении преподавателя.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleDeleteTeacher(int teacherId) {
    if (dbService.deleteTeacher(teacherId)) {
        json response;
        response["success"] = true;
        response["message"] = "Преподаватель успешно удалён!";
        return createJsonResponse(response.dump());
    } else {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Ошибка удаления преподавателя. Возможно, за ним закреплена группа.";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleAddTeacherSpecialization(const std::string& body) {
    try {
        json j = json::parse(body);
        
        if (!j.contains("teacher_id") || !j.contains("specialization_code")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поля 'teacher_id' и 'specialization_code' обязательны. Системная ошибка с клиентской стороны.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        int teacherId = j["teacher_id"];
        int specializationCode = j["specialization_code"];
        
        if (dbService.addTeacherSpecialization(teacherId, specializationCode)) {
            json response;
            response["success"] = true;
            response["message"] = "Специализация успешно добавлена преподавателю!";
            
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при добавлении специализации преподавателю.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleRemoveTeacherSpecialization(int teacherId, int specializationCode) {
    if (dbService.removeTeacherSpecialization(teacherId, specializationCode)) {
        json response;
        response["success"] = true;
        response["message"] = "Специализация преподавателя была удалена.";
        return createJsonResponse(response.dump());
    } else {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Ошибка удаления специализации преподавателя.";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleDeleteSpecialization(int specializationCode) {
    if (dbService.deleteSpecialization(specializationCode)) {
        json response;
        response["success"] = true;
        response["message"] = "Специализация успешно удалена.";
        return createJsonResponse(response.dump());
    } else {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Ошибка удаления специализации.";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::handleAddSpecialization(const std::string& body) {
    try {
        json j = json::parse(body);
        
        if (!j.contains("name") || j["name"].is_null()) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поле 'название' обязательное.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        std::string name = j["name"];
        int code = j.value("code", 0);
        
        Specialization spec;
        spec.name = name;
        spec.specializationCode = code;
        
        if (dbService.addSpecialization(spec)) {
            json response;
            response["success"] = true;
            response["message"] = "Специализация успешно добавлена.";
            response["code"] = code;
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка добавления специализации.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
        
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса, сработала try-catch инструкция.";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}