#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

// Portfolio handlers
std::string ApiService::handleAddPortfolio(const std::string& body, const std::string& sessionToken) {
    std::cout << "➕ Добавление портфолио..." << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        StudentPortfolio portfolio;
        
        // ОБРАБОТКА student_code - должно быть INTEGER
        if (j.contains("student_code")) {
            if (j["student_code"].is_number()) {
                portfolio.studentCode = j["student_code"].get<int>();
            } else if (j["student_code"].is_string()) {
                try {
                    portfolio.studentCode = std::stoi(j["student_code"].get<std::string>());
                } catch (const std::exception& e) {
                    std::cout << "❌ Ошибка преобразования student_code: " << e.what() << std::endl;
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат student_code\"}", 400);
                }
            } else {
                return createJsonResponse("{\"success\": false, \"error\": \"Неверный тип student_code\"}", 400);
            }
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Отсутствует student_code\"}", 400);
        }
        
        // ОБЯЗАТЕЛЬНЫЕ ПОЛЯ
        portfolio.date = j["date"];
        
        // ОБРАБОТКА decree - теперь INTEGER в БД
        if (j.contains("decree")) {
            if (j["decree"].is_number()) {
                portfolio.decree = j["decree"].get<int>(); // ИСПРАВЛЕНО: напрямую получаем int
            } else if (j["decree"].is_string()) {
                try {
                    portfolio.decree = std::stoi(j["decree"].get<std::string>()); // ИСПРАВЛЕНО: преобразуем строку в int
                } catch (const std::exception& e) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат decree\"}", 400);
                }
            } else {
                return createJsonResponse("{\"success\": false, \"error\": \"Неверный тип decree\"}", 400);
            }
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Отсутствует decree\"}", 400);
        }
        
        std::cout << "📦 Данные портфолио - student_code: " << portfolio.studentCode 
                  << ", date: " << portfolio.date 
                  << ", decree: " << portfolio.decree << std::endl;
        
        if (dbService.addPortfolio(portfolio)) {
            json response;
            response["success"] = true;
            response["message"] = "Портфолио успешно добавлено";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка добавления портфолио\"}", 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка добавления портфолио: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleUpdatePortfolio(const std::string& body, int portfolioId, const std::string& sessionToken) {
    std::cout << "🔄 Обновление портфолио ID: " << portfolioId << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        StudentPortfolio portfolio = dbService.getPortfolioById(portfolioId);
        
        if (portfolio.portfolioId == 0) {
            return createJsonResponse("{\"success\": false, \"error\": \"Портфолио не найдено\"}", 404);
        }
        
        // Обновляем только переданные поля
        if (j.contains("student_code")) {
            if (j["student_code"].is_number()) {
                portfolio.studentCode = j["student_code"].get<int>();
            } else if (j["student_code"].is_string()) {
                try {
                    portfolio.studentCode = std::stoi(j["student_code"].get<std::string>());
                } catch (const std::exception& e) {
                    std::cout << "❌ Ошибка преобразования student_code: " << e.what() << std::endl;
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат student_code\"}", 400);
                }
            }
        }
        
        if (j.contains("date")) {
            portfolio.date = j["date"];
        }
        
        if (j.contains("decree")) {
            if (j["decree"].is_number()) {
                portfolio.decree = j["decree"].get<int>(); // ИСПРАВЛЕНО: напрямую получаем int
            } else if (j["decree"].is_string()) {
                try {
                    portfolio.decree = std::stoi(j["decree"].get<std::string>()); // ИСПРАВЛЕНО: преобразуем строку в int
                } catch (const std::exception& e) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат decree\"}", 400);
                }
            }
        }
        
        std::cout << "📦 Обновленные данные портфолио - student_code: " << portfolio.studentCode 
                  << ", date: " << portfolio.date 
                  << ", decree: " << portfolio.decree << std::endl;
        
        if (dbService.updatePortfolio(portfolio)) {
            json response;
            response["success"] = true;
            response["message"] = "Портфолио успешно обновлено";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка обновления портфолио\"}", 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка обновления портфолио: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleDeletePortfolio(int portfolioId, const std::string& sessionToken) {
    std::cout << "🗑️ Удаление портфолио ID: " << portfolioId << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    if (dbService.deletePortfolio(portfolioId)) {
        json response;
        response["success"] = true;
        response["message"] = "Портфолио успешно удалено";
        return createJsonResponse(response.dump());
    } else {
        return createJsonResponse("{\"success\": false, \"error\": \"Ошибка удаления портфолио\"}", 500);
    }
}

// Event handlers - ИСПРАВЛЕННЫЕ (используем eventCategoryId вместо eventCategory)
std::string ApiService::handleAddEvent(const std::string& body, const std::string& sessionToken) {
    std::cout << "➕ Добавление события..." << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        Event event;
        
        // ОБЯЗАТЕЛЬНЫЕ ПОЛЯ из структуры БД
        if (j.contains("measure_code")) {
            if (j["measure_code"].is_number()) {
                event.measureCode = j["measure_code"].get<int>();
            } else if (j["measure_code"].is_string()) {
                try {
                    event.measureCode = std::stoi(j["measure_code"].get<std::string>());
                } catch (const std::exception& e) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат measure_code\"}", 400);
                }
            } else {
                return createJsonResponse("{\"success\": false, \"error\": \"Неверный тип measure_code\"}", 400);
            }
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Отсутствует measure_code\"}", 400);
        }
        
        event.eventCategory = j["event_category"];
        event.eventType = j["event_type"];
        event.startDate = j["start_date"];
        event.endDate = j["end_date"];
        event.location = j.value("location", "");
        event.lore = j.value("lore", "");
        
        std::cout << "📅 Данные события - measure_code: " << event.measureCode 
                  << ", category: " << event.eventCategory
                  << ", type: " << event.eventType << std::endl;
        
        if (dbService.addEvent(event)) {
            json response;
            response["success"] = true;
            response["message"] = "Событие успешно добавлено";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка добавления события\"}", 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка добавления события: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleUpdateEvent(const std::string& body, int eventId, const std::string& sessionToken) {
    std::cout << "🔄 Обновление события ID: " << eventId << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        Event event = dbService.getEventById(eventId);
        
        if (event.eventId == 0) {
            return createJsonResponse("{\"success\": false, \"error\": \"Событие не найдено\"}", 404);
        }
        
        // Обновляем только переданные поля
        if (j.contains("measure_code")) {
            if (j["measure_code"].is_number()) {
                event.measureCode = j["measure_code"].get<int>();
            } else if (j["measure_code"].is_string()) {
                try {
                    event.measureCode = std::stoi(j["measure_code"].get<std::string>());
                } catch (const std::exception& e) {
                    return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат measure_code\"}", 400);
                }
            }
        }
        
        if (j.contains("event_category")) event.eventCategory = j["event_category"];
        if (j.contains("event_type")) event.eventType = j["event_type"];
        if (j.contains("start_date")) event.startDate = j["start_date"];
        if (j.contains("end_date")) event.endDate = j["end_date"];
        if (j.contains("location")) event.location = j["location"];
        if (j.contains("lore")) event.lore = j["lore"];
        
        if (dbService.updateEvent(event)) {
            json response;
            response["success"] = true;
            response["message"] = "Событие успешно обновлено";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка обновления события\"}", 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка обновления события: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleDeleteEvent(int eventId, const std::string& sessionToken) {
    std::cout << "🗑️ Удаление события ID: " << eventId << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    if (dbService.deleteEvent(eventId)) {
        json response;
        response["success"] = true;
        response["message"] = "Событие успешно удалено";
        return createJsonResponse(response.dump());
    } else {
        return createJsonResponse("{\"success\": false, \"error\": \"Ошибка удаления события\"}", 500);
    }
}

// Event Category handlers
std::string ApiService::handleAddEventCategory(const std::string& body, const std::string& sessionToken) {
    std::cout << "➕ Добавление категории события..." << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        EventCategory category;
        
        category.name = j["name"];
        category.description = j.value("description", "");
        
        if (dbService.addEventCategory(category)) {
            json response;
            response["success"] = true;
            response["message"] = "Категория события успешно добавлена";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка добавления категории события\"}", 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка добавления категории события: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleUpdateEventCategory(const std::string& body, int categoryId, const std::string& sessionToken) {
    std::cout << "🔄 Обновление категории события ID: " << categoryId << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    try {
        json j = json::parse(body);
        EventCategory category = dbService.getEventCategoryById(categoryId);
        
        if (category.eventCategoryId == 0) {
            return createJsonResponse("{\"success\": false, \"error\": \"Категория события не найдена\"}", 404);
        }
        
        // Обновляем только переданные поля
        if (j.contains("name")) category.name = j["name"];
        if (j.contains("description")) category.description = j["description"];
        
        if (dbService.updateEventCategory(category)) {
            json response;
            response["success"] = true;
            response["message"] = "Категория события успешно обновлена";
            return createJsonResponse(response.dump());
        } else {
            return createJsonResponse("{\"success\": false, \"error\": \"Ошибка обновления категории события\"}", 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 Ошибка обновления категории события: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleDeleteEventCategory(int categoryId, const std::string& sessionToken) {
    std::cout << "🗑️ Удаление категории события ID: " << categoryId << std::endl;
    
    if (!validateSession(sessionToken)) {
        return createJsonResponse("{\"success\": false, \"error\": \"Unauthorized\"}", 401);
    }
    
    if (dbService.deleteEventCategory(categoryId)) {
        json response;
        response["success"] = true;
        response["message"] = "Категория события успешно удалена";
        return createJsonResponse(response.dump());
    } else {
        return createJsonResponse("{\"success\": false, \"error\": \"Ошибка удаления категории события\"}", 500);
    }
}