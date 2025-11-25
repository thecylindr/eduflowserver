#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>

// Event management
std::string DatabaseService::getCategoryNameById(int categoryId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return "";
    }
    
    std::string sql = "SELECT category FROM event_categories WHERE event_code = $1";
    const char* params[1] = { std::to_string(categoryId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    
    std::string categoryName = "";
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        categoryName = PQgetvalue(res, 0, 0);
        std::cout << "📚 Найдена категория: ID=" << categoryId << " -> '" << categoryName << "'" << std::endl;
    } else {
        std::cout << "❌ Категория с ID " << categoryId << " не найдена в БД" << std::endl;
    }
    
    PQclear(res);
    return categoryName;
}

std::vector<Event> DatabaseService::getEvents() {
    std::vector<Event> events;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return events;
    }
    
    std::string sql = "SELECT e.id, e.event_id, e.event_decode, e.event_type, "
                      "e.start_date, e.end_date, e.location, e.lore, ec.category "
                      "FROM event e "
                      "LEFT JOIN event_categories ec ON e.event_decode = ec.event_code";
    
    PGresult* res = PQexec(connection, sql.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        return events;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Event event;
        event.eventId = std::stoi(PQgetvalue(res, i, 0));
        event.measureCode = std::stoi(PQgetvalue(res, i, 1));
        event.eventDecode = std::stoi(PQgetvalue(res, i, 2));
        event.eventType = PQgetvalue(res, i, 3);
        event.startDate = PQgetvalue(res, i, 4);
        event.endDate = PQgetvalue(res, i, 5);
        event.location = PQgetvalue(res, i, 6);
        event.lore = PQgetvalue(res, i, 7);
        event.category = PQgetvalue(res, i, 8); 
        events.push_back(event);
    }
    
    PQclear(res);
    return events;
}

bool DatabaseService::addEvent(const Event& event) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    // Правильные параметры для вставки
    std::string sql = "INSERT INTO event (event_id, event_type, start_date, end_date, location, lore) "
                      "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id, event_decode";
    const char* params[6] = {
        std::to_string(event.measureCode).c_str(),  // event_id = measure_code из портфолио
        event.eventType.c_str(),
        event.startDate.c_str(),
        event.endDate.c_str(),
        event.location.c_str(),
        event.lore.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 6, NULL, params, NULL, NULL, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "❌ Ошибка добавления события: " << PQerrorMessage(connection) << std::endl;
        std::cerr << "   measureCode: " << event.measureCode << std::endl;
        std::cerr << "   SQL: " << sql << std::endl;
        PQclear(res);
        return false;
    }
    
    int newEventId = std::stoi(PQgetvalue(res, 0, 0));
    int newEventDecode = std::stoi(PQgetvalue(res, 0, 1));
    PQclear(res);
    
    std::cout << "✅ Событие добавлено, id: " << newEventId << ", decode: " << newEventDecode << std::endl;
    
    // Вставляем категорию, если указана (связываем с event_decode)
    if (!event.category.empty()) {
        std::string categorySql = "INSERT INTO event_categories (event_code, category) VALUES ($1, $2)";
        const char* categoryParams[2] = {
            std::to_string(newEventDecode).c_str(),
            event.category.c_str()
        };
        
        PGresult* categoryRes = PQexecParams(connection, categorySql.c_str(), 2, NULL, categoryParams, NULL, NULL, 0);
        bool categorySuccess = (PQresultStatus(categoryRes) == PGRES_COMMAND_OK);
        
        if (!categorySuccess) {
            std::cerr << "⚠️ Ошибка добавления категории: " << PQerrorMessage(connection) << std::endl;
            // Продолжаем, даже если категория не вставилась (событие сохранено)
        } else {
            std::cout << "✅ Категория сохранена: " << event.category << " для event_decode: " << newEventDecode << std::endl;
        }
        PQclear(categoryRes);
    }
    
    return true;
}

bool DatabaseService::updateEvent(const Event& event) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "UPDATE event SET event_type = $1, start_date = $2, end_date = $3, location = $4, lore = $5, event_id = $6 "
                      "WHERE id = $7";
    const char* params[7] = {
        event.eventType.c_str(),
        event.startDate.c_str(),
        event.endDate.c_str(),
        event.location.c_str(),
        event.lore.c_str(),
        std::to_string(event.measureCode).c_str(),
        std::to_string(event.eventId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 7, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    if (!success) {
        std::cerr << "❌ Ошибка обновления события: " << PQerrorMessage(connection) << std::endl;
        return false;
    }
    
    std::string decodeSql = "SELECT event_decode FROM event WHERE id = $1";
    const char* decodeParams[1] = { std::to_string(event.eventId).c_str() };
    PGresult* decodeRes = PQexecParams(connection, decodeSql.c_str(), 1, NULL, decodeParams, NULL, NULL, 0);
    int eventDecode = 0;
    if (PQresultStatus(decodeRes) == PGRES_TUPLES_OK && PQntuples(decodeRes) > 0) {
        eventDecode = std::stoi(PQgetvalue(decodeRes, 0, 0));
    }
    PQclear(decodeRes);
    
    if (eventDecode == 0) {
        std::cerr << "❌ Не удалось получить event_decode для события ID: " << event.eventId << std::endl;
        return false;
    }
    
    // Проверяем существование категории для данного event_decode
    std::string checkSql = "SELECT 1 FROM event_categories WHERE event_code = $1";
    const char* checkParams[1] = { std::to_string(eventDecode).c_str() };
    PGresult* checkRes = PQexecParams(connection, checkSql.c_str(), 1, NULL, checkParams, NULL, NULL, 0);
    bool categoryExists = (PQresultStatus(checkRes) == PGRES_TUPLES_OK && PQntuples(checkRes) > 0);
    PQclear(checkRes);
    
    if (categoryExists) {
        // Категория существует - ОБНОВЛЯЕМ
        if (!event.category.empty()) {
            std::string updateSql = "UPDATE event_categories SET category = $1 WHERE event_code = $2";
            const char* updateParams[2] = {
                event.category.c_str(),
                std::to_string(eventDecode).c_str()
            };
            
            PGresult* updateRes = PQexecParams(connection, updateSql.c_str(), 2, NULL, updateParams, NULL, NULL, 0);
            bool updateSuccess = (PQresultStatus(updateRes) == PGRES_COMMAND_OK);
            PQclear(updateRes);
            
            if (updateSuccess) {
                std::cout << "✅ Категория обновлена: '" << event.category << "' для event_decode: " << eventDecode << std::endl;
            } else {
                std::cerr << "⚠️ Ошибка обновления категории: " << PQerrorMessage(connection) << std::endl;
                return false;
            }
        } else {
            std::string deleteSql = "DELETE FROM event_categories WHERE event_code = $1";
            const char* deleteParams[1] = { std::to_string(eventDecode).c_str() };
            PGresult* deleteRes = PQexecParams(connection, deleteSql.c_str(), 1, NULL, deleteParams, NULL, NULL, 0);
            PQclear(deleteRes);
            std::cout << "✅ Категория удалена для event_decode: " << eventDecode << std::endl;
        }
    } else {
        if (!event.category.empty()) {
            std::string insertSql = "INSERT INTO event_categories (event_code, category) VALUES ($1, $2)";
            const char* insertParams[2] = {
                std::to_string(eventDecode).c_str(),
                event.category.c_str()
            };
            
            PGresult* insertRes = PQexecParams(connection, insertSql.c_str(), 2, NULL, insertParams, NULL, NULL, 0);
            bool insertSuccess = (PQresultStatus(insertRes) == PGRES_COMMAND_OK);
            PQclear(insertRes);
            
            if (insertSuccess) {
                std::cout << "✅ Категория добавлена: '" << event.category << "' для event_decode: " << eventDecode << std::endl;
            } else {
                std::cerr << "⚠️ Ошибка добавления категории: " << PQerrorMessage(connection) << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

bool DatabaseService::deleteEvent(int eventId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM event WHERE id = $1";
    const char* params[1] = { std::to_string(eventId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка удаления события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

Event DatabaseService::getEventById(int eventId) {
    Event event;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return event;
    }
    
    std::string sql = R"(
        SELECT e.id, e.event_id, e.event_decode, e.event_type, 
            e.start_date, e.end_date, e.location, e.lore,
            ec.category
        FROM event e
        LEFT JOIN event_categories ec ON e.event_decode = ec.event_code
        WHERE e.id = $1
    )";
    
    const char* params[1] = { std::to_string(eventId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return event;
    }
    
    event.eventId = std::stoi(PQgetvalue(res, 0, 0));
    event.measureCode = std::stoi(PQgetvalue(res, 0, 1));
    event.eventDecode = std::stoi(PQgetvalue(res, 0, 2));
    event.eventType = PQgetvalue(res, 0, 3);
    event.startDate = PQgetvalue(res, 0, 4);
    event.endDate = PQgetvalue(res, 0, 5);
    event.location = PQgetvalue(res, 0, 6);
    event.lore = PQgetvalue(res, 0, 7);
    event.category = PQgetvalue(res, 0, 8);
    
    PQclear(res);
    return event;
}

// Event Category management
std::vector<EventCategory> DatabaseService::getEventCategories() {
    std::vector<EventCategory> categories;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return categories;
    }
    
    std::string sql = "SELECT event_code, category FROM event_categories";
    PGresult* res = PQexec(connection, sql.c_str());
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка выполнения запроса категорий событий: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return categories;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        EventCategory category;
        
        if (!PQgetisnull(res, i, 0)) {
            const char* eventCodeStr = PQgetvalue(res, i, 0);
            try {
                category.eventCode = std::stoi(eventCodeStr);
            } catch (const std::exception& e) {
                std::cerr << "❌ Ошибка преобразования event_code в число: " << eventCodeStr << " - " << e.what() << std::endl;
                continue; // Пропускаем некорректную запись
            }
        } else {
            std::cerr << "⚠️ Обнаружен NULL в event_code, пропускаем запись" << std::endl;
            continue;
        }
        
        if (!PQgetisnull(res, i, 1)) {
            category.category = PQgetvalue(res, i, 1);
        } else {
            category.category = "";
        }
        
        categories.push_back(category);
    }
    
    PQclear(res);
    std::cout << "✅ Получено категорий событий: " << categories.size() << std::endl;
    return categories;
}

bool DatabaseService::addEventCategory(const EventCategory& category) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "INSERT INTO event_categories (event_code, category) VALUES ($1, $2)";
    const char* params[2] = {
        std::to_string(category.eventCode).c_str(),
        category.category.c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 2, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (success) {
        std::cout << "✅ Категория события добавлена: event_code=" << category.eventCode 
                << " - " << category.category << std::endl;
    } else {
        std::cerr << "Ошибка добавления категории события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

EventCategory DatabaseService::getEventCategoryByCode(int eventCode) {
    EventCategory category;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return category;
    }
    
    std::string sql = "SELECT event_code, category FROM event_categories WHERE event_code = $1";
    const char* params[1] = { std::to_string(eventCode).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return category;
    }
    
    category.eventCode = std::stoi(PQgetvalue(res, 0, 0));
    category.category = PQgetvalue(res, 0, 1);
    
    PQclear(res);
    return category;
}

bool DatabaseService::updateEventCategory(const EventCategory& category) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "UPDATE event_categories SET category = $1 WHERE event_code = $2";
    const char* params[2] = {
        category.category.c_str(),
        std::to_string(category.eventCode).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 2, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка обновления категории события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::deleteEventCategory(int eventCode) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM event_categories WHERE event_code = $1";
    const char* params[1] = { std::to_string(eventCode).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка удаления категории события: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}