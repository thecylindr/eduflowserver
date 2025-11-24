#include "api/ApiService.h"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

bool ApiService::isSafeNewsFilename(const std::string& filename) {
    // Проверяем, что filename безопасный
    if (filename.empty() || filename.length() > 100) {
        return false;
    }
    
    // Разрешаем только буквы, цифры, подчеркивание, дефисы и точку
    for (char c : filename) {
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '.' && c != ' ') {
            return false;
        }
    }
    
    // Разрешаем только .json файлы
    if (filename.substr(filename.find_last_of('.')) != ".json") {
        return false;
    }
    
    // Запрещаем путь с ..
    if (filename.find("..") != std::string::npos) {
        return false;
    }
    
    return true;
}

std::string ApiService::handleGetNewsList() {
    try {
        // Создаем папку news если не существует
        std::filesystem::create_directories("news");
        
        std::vector<json> newsList;
        
        for (const auto& entry : std::filesystem::directory_iterator("news")) {
            // Ищем только .json файлы
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                
                // Читаем JSON файл
                std::ifstream file(entry.path());
                json newsJson;
                try {
                    file >> newsJson;
                    
                    // Извлекаем заголовок из JSON
                    std::string title = "Без заголовка";
                    if (newsJson.contains("title") && newsJson["title"].is_string()) {
                        title = newsJson["title"];
                    }
                    
                    std::string date = "Неизвестно";
                    if (newsJson.contains("date") && newsJson["date"].is_string()) {
                        date = newsJson["date"];
                    }
                    
                    std::string author = "Неизвестен";
                    if (newsJson.contains("author") && newsJson["author"].is_string()) {
                        author = newsJson["author"];
                    }
                    
                    // Добавляем в список
                    newsList.push_back({
                        {"filename", filename},
                        {"title", title},
                        {"date", date},
                        {"author", author}
                    });
                    
                } catch (const std::exception& e) {
                    std::cout << "[ERROR] Error parsing JSON news: " << filename << " - " << e.what() << std::endl;
                    // Добавляем с заголовком об ошибке
                    newsList.push_back({
                        {"filename", filename},
                        {"title", "Ошибка чтения файла"},
                        {"date", "Неизвестно"},
                        {"author", "Неизвестен"}
                    });
                }
            }
        }
        
        // Сортируем по дате (новые сначала) или по имени файла
        std::sort(newsList.begin(), newsList.end(), [](const auto& a, const auto& b) {
            // Пытаемся сравнить по дате, если есть
            std::string dateA = a.value("date", "");
            std::string dateB = b.value("date", "");
            if (!dateA.empty() && !dateB.empty()) {
                return dateA > dateB;
            }
            // Иначе по имени файла
            return a.value("filename", "") > b.value("filename", "");
        });
        
        json response;
        response["success"] = true;
        response["data"] = newsList;
        
        std::cout << "[NEWS] Returned " << newsList.size() << " JSON news items" << std::endl;
        return createJsonResponse(response.dump());
        
    } catch (const std::exception& e) {
        std::cout << "[ERROR] Error reading news directory: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Failed to read news\"}", 500);
    }
}

std::string ApiService::handleGetNews(const std::string& filename) {
    std::cout << "🔍 Обработка запроса новости: " << filename << std::endl;
    
    // Более гибкая проверка имени файла
    if (filename.empty() || filename.length() > 200) {
        std::cout << "❌ Неверное имя файла: пустое или слишком длинное" << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid filename\"}", 400);
    }
    
    // Проверяем наличие опасных символов и path traversal
    if (filename.find("..") != std::string::npos ||
        filename.find("/") != std::string::npos ||
        filename.find("\\") != std::string::npos) {
        std::cout << "❌ Опасное имя файла: " << filename << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid filename\"}", 400);
    }
    
    // Проверяем расширение .json
    if (filename.length() < 5 || filename.substr(filename.length() - 5) != ".json") {
        std::cout << "❌ Неверное расширение файла: " << filename << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid file format\"}", 400);
    }
    
    std::string filepath = "news/" + filename;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cout << "❌ Файл новости не найден: " << filepath << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"News not found\"}", 404);
    }
    
    try {
        json newsJson;
        file >> newsJson;
        
        // Добавляем информацию о файле
        newsJson["filename"] = filename;
        
        // Получаем дату изменения файла
        auto ftime = std::filesystem::last_write_time(filepath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        std::time_t lastWriteTime = std::chrono::system_clock::to_time_t(sctp);
        
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%d.%m.%Y %H:%M", std::localtime(&lastWriteTime));
        newsJson["lastModified"] = timeStr;
        
        json response;
        response["success"] = true;
        response["data"] = newsJson;
        
        std::cout << "✅ Новость успешно загружена: " << filename << std::endl;
        return createJsonResponse(response.dump());
        
    } catch (const std::exception& e) {
        std::cout << "❌ Ошибка парсинга JSON новости: " << e.what() << std::endl;
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid JSON format\"}", 400);
    }
}