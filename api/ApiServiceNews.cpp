#include "api/ApiService.h"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;

bool ApiService::isSafeNewsFilename(const std::string& filename) {
    if (filename.empty() || filename.length() > 100) {
        return false;
    }
    
    for (char c : filename) {
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '.' && c != ' ') {
            return false;
        }
    }
    
    if (filename.substr(filename.find_last_of('.')) != ".json") {
        return false;
    }
    
    if (filename.find("..") != std::string::npos) {
        return false;
    }
    
    return true;
}

std::string ApiService::handleGetNewsList() {
    try {
        std::filesystem::create_directories("news");
        
        std::vector<json> newsList;
        
        for (const auto& entry : std::filesystem::directory_iterator("news")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                
                std::ifstream file(entry.path());
                json newsJson;
                try {
                    file >> newsJson;
                    
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
                    
                    newsList.push_back({
                        {"filename", filename},
                        {"title", title},
                        {"date", date},
                        {"author", author}
                    });
                    
                } catch (const std::exception& e) {
                    newsList.push_back({
                        {"filename", filename},
                        {"title", "Ошибка чтения файла"},
                        {"date", "Неизвестно"},
                        {"author", "Неизвестен"}
                    });
                }
            }
        }
        
        std::sort(newsList.begin(), newsList.end(), [](const auto& a, const auto& b) {
            std::string dateA = a.value("date", "");
            std::string dateB = b.value("date", "");
            if (!dateA.empty() && !dateB.empty()) {
                return dateA > dateB;
            }
            return a.value("filename", "") > b.value("filename", "");
        });
        
        json response;
        response["success"] = true;
        response["data"] = newsList;
        
        return createJsonResponse(response.dump());
        
    } catch (const std::exception& e) {
        return createJsonResponse("{\"success\": false, \"error\": \"Failed to read news\"}", 500);
    }
}

std::string ApiService::handleGetNews(const std::string& filename) {
    if (filename.empty() || filename.length() > 200) {
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid filename\"}", 400);
    }
    
    if (filename.find("..") != std::string::npos ||
        filename.find("/") != std::string::npos ||
        filename.find("\\") != std::string::npos) {
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid filename\"}", 400);
    }
    
    if (filename.length() < 5 || filename.substr(filename.length() - 5) != ".json") {
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid file format\"}", 400);
    }
    
    std::string filepath = "news/" + filename;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return createJsonResponse("{\"success\": false, \"error\": \"Новости не найдены.\"}", 404);
    }
    
    try {
        json newsJson;
        file >> newsJson;
        
        newsJson["filename"] = filename;
        
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
        
        return createJsonResponse(response.dump());
        
    } catch (const std::exception& e) {
        return createJsonResponse("{\"success\": false, \"error\": \"Invalid JSON format\"}", 400);
    }
}