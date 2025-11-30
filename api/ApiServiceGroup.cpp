#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

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
            response["message"] = "Группа успешно добавлена!";
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка добавления группы.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный запрос на сервер.";
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
            errorResponse["error"] = "Группа не найдена.";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        if (j.contains("name")) group.name = j["name"];
        if (j.contains("student_count")) group.studentCount = j["student_count"];
        if (j.contains("teacher_id")) group.teacherId = j["teacher_id"];
        
        if (dbService.updateGroup(group)) {
            json response;
            response["success"] = true;
            response["message"] = "Группа успешно обновлена!";
            return createJsonResponse(response.dump());
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка обновления группы.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный запрос на сервер.";
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleDeleteGroup(int groupId) {
    if (dbService.deleteGroup(groupId)) {
        json response;
        response["success"] = true;
        response["message"] = "Группа успешно удалена.";
        return createJsonResponse(response.dump());
    } else {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Ошибка удаления группы. Возможно, в ней ещё находятся студенты.";
        return createJsonResponse(errorResponse.dump(), 500);
    }
}

std::string ApiService::getEventsJson(const std::string& sessionToken) {
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