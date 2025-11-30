#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

std::string ApiService::handleAddPortfolio(const std::string& body) {
    try {
        json j = json::parse(body);
        StudentPortfolio portfolio;
        
        if (j.contains("student_code")) {
            if (j["student_code"].is_number()) {
                portfolio.studentCode = j["student_code"].get<int>();
            } else if (j["student_code"].is_string()) {
                try {
                    portfolio.studentCode = std::stoi(j["student_code"].get<std::string>());
                } catch (const std::exception& e) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат код студента.\"}", 400);
                }
            } else {
                return createJsonResponse("{\"success\": false, \"error\": \"Неверный тип код студента.\"}", 400);
            }
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Отсутствует код студента.\"}", 400);
        }
        
        portfolio.date = j["date"];
        
        if (j.contains("decree")) {
            if (j["decree"].is_number()) {
                portfolio.decree = j["decree"].get<int>();
            } else if (j["decree"].is_string()) {
                try {
                    portfolio.decree = std::stoi(j["decree"].get<std::string>());
                } catch (const std::exception& e) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат указа.\"}", 400);
                }
            } else {
                return createJsonResponse("{\"success\": false, \"error\": \"Неверный тип номер указа.\"}", 400);
            }
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Отсутствует номер указа\"}", 400);
        }
        
        if (dbService.addPortfolio(portfolio)) {
            json response;
            response["success"] = true;
            response["message"] = "Портфолио успешно добавлено";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка добавления портфолио\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleUpdatePortfolio(const std::string& body, int portfolioId) {
    try {
        json j = json::parse(body);
        StudentPortfolio portfolio = dbService.getPortfolioById(portfolioId);
        
        if (portfolio.portfolioId == 0) {
            return createJsonResponse("{\"success\": false, \"error\": \"Портфолио не найдено\"}", 404);
        }
        
        if (j.contains("student_code")) {
            if (j["student_code"].is_number()) {
                portfolio.studentCode = j["student_code"].get<int>();
            } else if (j["student_code"].is_string()) {
                try {
                    portfolio.studentCode = std::stoi(j["student_code"].get<std::string>());
                } catch (const std::exception& e) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат кода студента.\"}", 400);
                }
            }
        }
        
        if (j.contains("date")) {
            portfolio.date = j["date"];
        }
        
        if (j.contains("decree")) {
            if (j["decree"].is_number()) {
                portfolio.decree = j["decree"].get<int>();
            } else if (j["decree"].is_string()) {
                try {
                    portfolio.decree = std::stoi(j["decree"].get<std::string>());
                } catch (const std::exception& e) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат номера указа, должно быть число.\"}", 400);
                }
            }
        }
        
        if (dbService.updatePortfolio(portfolio)) {
            json response;
            response["success"] = true;
            response["message"] = "Портфолио успешно обновлено!";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка обновления портфолио\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleDeletePortfolio(int portfolioId) {
    if (dbService.deletePortfolio(portfolioId)) {
        json response;
        response["success"] = true;
        response["message"] = "Портфолио успешно удалено!";
        return createJsonResponse(response.dump());
    } else {
        return createJsonResponse("{\"success\": false, \"error\": \"Ошибка удаления портфолио\"}", 500);
    }
}

std::string ApiService::handleAddEvent(const std::string& body) {
    try {
        json j = json::parse(body);
        Event event;
        
        if (j.contains("measureCode")) {
            event.measureCode = j["measureCode"];
        } else {
            event.measureCode = j.value("event_id", j.value("event_code", 0));
        }
        
        if (!j.contains("event_type") || !j.contains("start_date")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поля дат начала и конца обязательны.";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        event.eventType = j["event_type"];
        event.startDate = j["start_date"];
        event.endDate = j.value("end_date", "");
        event.location = j.value("location", "");
        event.lore = j.value("lore", "");
        
        if (event.measureCode <= 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный код портфолио: должен быть положительным числом";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        if (!dbService.portfolioExists(event.measureCode)) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Портфолио с measure_code " + std::to_string(event.measureCode) + " не найдено";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        if (j.contains("category") && !j["category"].is_null()) {
            event.category = j["category"];
        }
        
        if (dbService.addEvent(event)) {
            json response;
            response["success"] = true;
            response["message"] = "Событие успешно добавлено!";
            
            return createJsonResponse(response.dump(), 201);
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при добавлении события.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

bool DatabaseService::portfolioExists(int measureCode) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "SELECT 1 FROM student_portfolio WHERE measure_code = $1";
    const char* params[1] = { std::to_string(measureCode).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool exists = (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0);
    
    PQclear(res);
    return exists;
}

std::string ApiService::handleUpdateEvent(const std::string& body, int eventId) {
    try {
        json j = json::parse(body);
        
        Event event = dbService.getEventById(eventId);
        
        if (event.eventId == 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Событие не найдено.";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        if (j.contains("measure_code")) {
            if (j["measure_code"].is_number()) {
                int newMeasureCode = j["measure_code"].get<int>();
                if (!dbService.portfolioExists(newMeasureCode)) {
                    json errorResponse;
                    errorResponse["success"] = false;
                    errorResponse["error"] = "Портфолио с кодом портфолио " + std::to_string(newMeasureCode) + " не найдено.";
                    return createJsonResponse(errorResponse.dump(), 404);
                }
                event.measureCode = newMeasureCode;
            }
        }
        
        if (j.contains("event_type")) event.eventType = j["event_type"];
        if (j.contains("start_date")) event.startDate = j["start_date"];
        if (j.contains("end_date")) event.endDate = j["end_date"];
        if (j.contains("location")) event.location = j["location"];
        if (j.contains("lore")) event.lore = j["lore"];
        
        if (j.contains("category")) {
            if (!j["category"].is_null()) {
                event.category = j["category"].get<std::string>();
            } else {
                event.category = "";
            }
        }
        
        if (dbService.updateEvent(event)) {
            json response;
            response["success"] = true;
            response["message"] = "Событие успешно обновлено.";
            
            return createJsonResponse(response.dump());
        } else {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при обновлении события.";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleDeleteEvent(int eventId) {
    if (dbService.deleteEvent(eventId)) {
        json response;
        response["success"] = true;
        response["message"] = "Событие успешно удалено.";
        return createJsonResponse(response.dump());
    } else {
        return createJsonResponse("{\"success\": false, \"error\": \"Ошибка удаления события\"}", 500);
    }
}

std::string ApiService::handleAddEventCategory(const std::string& body) {
    try {
        json j = json::parse(body);
        EventCategory category;
        
        if (!j.contains("event_code") || !j.contains("category")) {
            return createJsonResponse("{\"success\": false, \"error\": \"Поля 'event_code' и 'category' обязательны. Системная ошибка клиента.\"}", 400);
        }
        
        category.eventCode = j["event_code"];
        category.category = j["category"];
        
        if (category.category.length() > 64) {
            return createJsonResponse("{\"success\": false, \"error\": \"Полное наименование не должно превышать 64 символа.\"}", 400);
        }
        
        if (dbService.addEventCategory(category)) {
            json response;
            response["success"] = true;
            response["message"] = "Категория события успешно добавлена";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка добавления категории события\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleUpdateEventCategory(const std::string& body, int eventCode) {
    try {
        json j = json::parse(body);
        EventCategory category = dbService.getEventCategoryByCode(eventCode);
        
        if (category.eventCode == 0) {
            return createJsonResponse("{\"success\": false, \"error\": \"Категория события не найдена\"}", 404);
        }
        
        if (j.contains("category")) {
            category.category = j["category"];
            if (category.category.length() > 64) {
                return createJsonResponse("{\"success\": false, \"error\": \"Полное наименование не должно превышать 64 символа\"}", 400);
            }
        }
        
        if (dbService.updateEventCategory(category)) {
            json response;
            response["success"] = true;
            response["message"] = "Категория события успешно обновлена";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка обновления категории события\"}", 500);
        }
    } catch (const std::exception& e) {
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleDeleteEventCategory(int eventCode) {
    if (dbService.deleteEventCategory(eventCode)) {
        json response;
        response["success"] = true;
        response["message"] = "Категория события успешно удалена";
        return createJsonResponse(response.dump());
    } else {
        return createJsonResponse("{\"success\": false, \"error\": \"Ошибка удаления категории события\"}", 500);
    }
}