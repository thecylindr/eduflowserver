#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

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
        
        // Проверка номера телефона
        if (!student.phoneNumber.empty() && !ApiService::isValidPhoneNumber(student.phoneNumber)) {
            std::cout << "❌ Invalid phone number format for student: " << student.phoneNumber << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Номер телефона должен содержать ровно 11 цифр";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (dbService.addStudent(student)) {
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
        
        // Проверка номера телефона
        if (!newStudent.phoneNumber.empty() && !ApiService::isValidPhoneNumber(newStudent.phoneNumber)) {
            std::cout << "❌ Invalid phone number format for student: " << newStudent.phoneNumber << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Номер телефона должен содержать ровно 11 цифр";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
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
        // Уменьшение счётчика группы
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