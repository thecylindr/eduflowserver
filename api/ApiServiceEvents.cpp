#include "api/ApiService.h"
#include "json.hpp"
#include <iostream>

using json = nlohmann::json;

// Portfolio handlers
std::string ApiService::handleAddPortfolio(const std::string& body) {
    std::cout << "➕ Добавление портфолио..." << std::endl;
    
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

std::string ApiService::handleUpdatePortfolio(const std::string& body, int portfolioId) {
    std::cout << "🔄 Обновление портфолио ID: " << portfolioId << std::endl;
    
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

std::string ApiService::handleDeletePortfolio(int portfolioId) {
    std::cout << "🗑️ Удаление портфолио ID: " << portfolioId << std::endl;
    
    if (dbService.deletePortfolio(portfolioId)) {
        json response;
        response["success"] = true;
        response["message"] = "Портфолио успешно удалено";
        return createJsonResponse(response.dump());
    } else {
        return createJsonResponse("{\"success\": false, \"error\": \"Ошибка удаления портфолио\"}", 500);
    }
}

std::string ApiService::handleAddEvent(const std::string& body) {
    std::cout << "🔄 Обработка добавления события..." << std::endl;
    std::cout << "📦 Тело запроса: " << body << std::endl;
    
    try {
        json j = json::parse(body);
        Event event;
        
        // ИСПРАВЛЕНИЕ: Получаем measure_code вместо event_id
        if (j.contains("measureCode")) {
            event.measureCode = j["measureCode"];
            std::cout << "✅ measureCode из запроса: " << event.measureCode << std::endl;
        } else {
            // ИСПРАВЛЕНИЕ: Пробуем альтернативные поля
            event.measureCode = j.value("event_id", j.value("event_code", 0));
            std::cout << "⚠️ measureCode не найден, используем альтернативы: " << event.measureCode << std::endl;
        }
        
        // Обязательные поля
        if (!j.contains("event_type") || !j.contains("start_date")) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Поля 'event_type' и 'start_date' обязательны";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        event.eventType = j["event_type"];
        event.startDate = j["start_date"];
        event.endDate = j.value("end_date", "");
        event.location = j.value("location", "");
        event.lore = j.value("lore", "");
        
        // ИСПРАВЛЕНИЕ: Проверяем существование портфолио
        if (event.measureCode <= 0) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Неверный measureCode: должен быть положительным числом";
            return createJsonResponse(errorResponse.dump(), 400);
        }
        
        // Проверяем существование портфолио
        if (!dbService.portfolioExists(event.measureCode)) {
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Портфолио с measure_code " + std::to_string(event.measureCode) + " не найдено";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        // УПРОЩЕНИЕ: Просто сохраняем категорию в объект Event
        if (j.contains("category") && !j["category"].is_null()) {
            event.category = j["category"];
            std::cout << "🏷️ Категория события: " << event.category << std::endl;
        }
        
        std::cout << "📅 Добавление события для measure_code: " << event.measureCode << std::endl;
        
        // Добавляем событие в БД
        if (dbService.addEvent(event)) {
            std::cout << "✅ Событие успешно добавлено" << std::endl;
            
            json response;
            response["success"] = true;
            response["message"] = "Событие успешно добавлено!";
            
            return createJsonResponse(response.dump(), 201);
        } else {
            std::cout << "❌ Ошибка при добавлении события в БД" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при добавлении события";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в handleAddEvent: " << e.what() << std::endl;
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
    std::cout << "🔄 Обработка обновления события ID: " << eventId << std::endl;
    
    try {
        json j = json::parse(body);
        
        // Получаем текущие данные события
        Event event = dbService.getEventById(eventId);
        
        if (event.eventId == 0) {
            std::cout << "❌ Событие не найдено: " << eventId << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Событие не найдено";
            return createJsonResponse(errorResponse.dump(), 404);
        }
        
        std::cout << "📅 Обновление события ID: " << eventId << std::endl;
        
        // КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ: Обновляем measureCode (связку с портфолио)
        if (j.contains("measure_code")) {
            if (j["measure_code"].is_number()) {
                int newMeasureCode = j["measure_code"].get<int>();
                // Проверяем существование портфолио
                if (!dbService.portfolioExists(newMeasureCode)) {
                    json errorResponse;
                    errorResponse["success"] = false;
                    errorResponse["error"] = "Портфолио с measure_code " + std::to_string(newMeasureCode) + " не найдено";
                    return createJsonResponse(errorResponse.dump(), 404);
                }
                event.measureCode = newMeasureCode;
                std::cout << "🔄 Обновлена связка с портфолио: " << event.measureCode << std::endl;
            }
        }
        
        // Обновляем остальные поля
        if (j.contains("event_type")) event.eventType = j["event_type"];
        if (j.contains("start_date")) event.startDate = j["start_date"];
        if (j.contains("end_date")) event.endDate = j["end_date"];
        if (j.contains("location")) event.location = j["location"];
        if (j.contains("lore")) event.lore = j["lore"];
        
        // ИСПРАВЛЕНИЕ: Обработка категории
        if (j.contains("category") && !j["category"].is_null()) {
            event.category = j["category"];
            std::cout << "🏷️ Обновление категории: " << event.category << std::endl;
        } else {
            event.category = ""; // Очищаем категорию если не передана
        }
        
        std::cout << "📦 Обновленные данные события - ID: " << event.eventId 
                  << ", measureCode: " << event.measureCode 
                  << ", eventType: " << event.eventType 
                  << ", category: " << event.category << std::endl;
        
        if (dbService.updateEvent(event)) {
            std::cout << "✅ Событие успешно обновлено" << std::endl;
            
            json response;
            response["success"] = true;
            response["message"] = "Событие успешно обновлено";
            
            return createJsonResponse(response.dump());
        } else {
            std::cout << "❌ Ошибка при обновлении события" << std::endl;
            json errorResponse;
            errorResponse["success"] = false;
            errorResponse["error"] = "Ошибка при обновлении события";
            return createJsonResponse(errorResponse.dump(), 500);
        }
    } catch (const std::exception& e) {
        std::cout << "💥 EXCEPTION в handleUpdateEvent: " << e.what() << std::endl;
        json errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Неверный формат запроса: " + std::string(e.what());
        return createJsonResponse(errorResponse.dump(), 400);
    }
}

std::string ApiService::handleDeleteEvent(int eventId) {
    std::cout << "🗑️ Удаление события ID: " << eventId << std::endl;
    
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
std::string ApiService::handleAddEventCategory(const std::string& body) {
    std::cout << "➕ Добавление категории события..." << std::endl;
    
    try {
        json j = json::parse(body);
        EventCategory category;
        
        // Проверяем обязательные поля
        if (!j.contains("event_code") || !j.contains("category")) {
            return createJsonResponse("{\"success\": false, \"error\": \"Поля 'event_code' и 'category' обязательны\"}", 400);
        }
        
        category.eventCode = j["event_code"];
        category.category = j["category"];
        
        // Валидация длины
        if (category.category.length() > 64) {
            return createJsonResponse("{\"success\": false, \"error\": \"Полное наименование (category) не должно превышать 64 символа\"}", 400);
        }
        
        std::cout << "📝 Данные категории события - event_code: " << category.eventCode 
                  << ", category: " << category.category << std::endl;
        
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

std::string ApiService::handleUpdateEventCategory(const std::string& body, int eventCode) {
    std::cout << "🔄 Обновление категории события для event_code: " << eventCode << std::endl;
    
    try {
        json j = json::parse(body);
        EventCategory category = dbService.getEventCategoryByCode(eventCode);
        
        if (category.eventCode == 0) {
            return createJsonResponse("{\"success\": false, \"error\": \"Категория события не найдена\"}", 404);
        }
        
        // Обновляем только переданные поля
        if (j.contains("category")) {
            category.category = j["category"];
            if (category.category.length() > 64) {
                return createJsonResponse("{\"success\": false, \"error\": \"Полное наименование (category) не должно превышать 64 символа\"}", 400);
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
        std::cout << "💥 Ошибка обновления категории события: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Неверный формат запроса\"}", 400);
    }
}

std::string ApiService::handleDeleteEventCategory(int eventCode) {
    std::cout << "🗑️ Удаление категории события для event_code: " << eventCode << std::endl;
    
    if (dbService.deleteEventCategory(eventCode)) {
        json response;
        response["success"] = true;
        response["message"] = "Категория события успешно удалена";
        return createJsonResponse(response.dump());
    } else {
        return createJsonResponse("{\"success\": false, \"error\": \"Ошибка удаления категории события\"}", 500);
    }
}