#include "api/ApiService.h"
#include "json.hpp"

using json = nlohmann::json;

// Teacher CRUD operations
std::string ApiService::handleAddTeacher(const std::string& body, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        Teacher teacher;
        
        teacher.lastName = j["last_name"];
        teacher.firstName = j["first_name"];
        teacher.middleName = j.value("middle_name", "");
        teacher.experience = j["experience"];
        teacher.specialization = j["specialization"];
        teacher.email = j.value("email", "");
        teacher.phoneNumber = j.value("phone_number", "");
        
        if (dbService.addTeacher(teacher)) {
            return createJsonResponse("{\"message\": \"Teacher added successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Failed to add teacher\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format: " + std::string(e.what()) + "\"}", 400);
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
            return createJsonResponse("{\"error\": \"Teacher not found\"}", 404);
        }
        
        if (j.contains("last_name")) teacher.lastName = j["last_name"];
        if (j.contains("first_name")) teacher.firstName = j["first_name"];
        if (j.contains("middle_name")) teacher.middleName = j["middle_name"];
        if (j.contains("experience")) teacher.experience = j["experience"];
        if (j.contains("specialization")) teacher.specialization = j["specialization"];
        if (j.contains("email")) teacher.email = j["email"];
        if (j.contains("phone_number")) teacher.phoneNumber = j["phone_number"];
        
        if (dbService.updateTeacher(teacher)) {
            return createJsonResponse("{\"message\": \"Teacher updated successfully\"}");
        } else {
            return createJsonResponse("{\"error\": \"Failed to update teacher\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

std::string ApiService::handleDeleteTeacher(int teacherId, const std::string& sessionToken) {
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"error\": \"Unauthorized\"}", 401);
    }
    
    if (dbService.deleteTeacher(teacherId)) {
        return createJsonResponse("{\"message\": \"Teacher deleted successfully\"}");
    } else {
        return createJsonResponse("{\"error\": \"Failed to delete teacher\"}", 500);
    }
}

// Student CRUD operations
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

// Group CRUD operations
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

// Portfolio operations
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
        portfolio.passportSeries = j["passport_series"];
        portfolio.passportNumber = j["passport_number"];
        
        if (dbService.addPortfolio(portfolio)) {
            return createJsonResponse("{\"message\": \"Portfolio item added successfully\"}", 201);
        } else {
            return createJsonResponse("{\"error\": \"Failed to add portfolio item\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"error\": \"Invalid request format\"}", 400);
    }
}

// Event operations
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

// Profile operations
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
        
        // If password is being updated, hash the new password
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