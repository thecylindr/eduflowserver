// LocaleManager.h
#ifndef LOCALEMANAGER_H
#define LOCALEMANAGER_H

#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#else
#include <curl/curl.h>
#endif

class LocaleManager {
public:
    static bool checkAndDownloadLocales() {
        std::string langDir = "lang";
        
        // Создаем папку lang если её нет
        if (!std::filesystem::exists(langDir)) {
            std::filesystem::create_directory(langDir);
        }
        
        bool enExists = std::filesystem::exists(langDir + "/locale_en.json");
        bool ruExists = std::filesystem::exists(langDir + "/locale_ru.json");
        
        // Если оба файла существуют, ничего не делаем
        if (enExists && ruExists) {
            std::cout << "✅ Файлы локализации найдены" << std::endl;
            return true;
        }
        
        std::cout << "🔍 Проверка интернет соединения..." << std::endl;
        
        if (!checkInternetConnection()) {
            std::cout << "❌ Нет интернет соединения. Файлы локализации не могут быть загружены." << std::endl;
            return false;
        }
        
        std::cout << "✅ Интернет соединение доступно" << std::endl;
        std::cout << "📥 Загрузка файлов локализации..." << std::endl;
        
        // Загружаем недостающие файлы
        bool success = true;
        if (!enExists) {
            std::cout << "⬇️  Загрузка locale_en.json..." << std::endl;
            if (!downloadFile("https://gitflic.ru/project/cylindr/eduflowserver/raw?file=lang%2Flocale_en.json", 
                            langDir + "/locale_en.json")) {
                std::cout << "❌ Ошибка загрузки locale_en.json" << std::endl;
                success = false;
            }
        }
        
        if (!ruExists) {
            std::cout << "⬇️  Загрузка locale_ru.json..." << std::endl;
            if (!downloadFile("https://gitflic.ru/project/cylindr/eduflowserver/raw?file=lang%2Flocale_ru.json", 
                            langDir + "/locale_ru.json")) {
                std::cout << "❌ Ошибка загрузки locale_ru.json" << std::endl;
                success = false;
            }
        }
        
        if (success) {
            std::cout << "✅ Файлы локализации успешно загружены" << std::endl;
        } else {
            std::cout << "⚠️  Некоторые файлы локализации не были загружены" << std::endl;
        }
        
        return success;
    }

private:
    static bool checkInternetConnection() {
#ifdef _WIN32
        return InternetCheckConnectionA("https://gitflic.ru", FLAG_ICC_FORCE_CONNECTION, 0);
#else
        // Для Linux используем системный вызов ping или проверку через curl
        return system("ping -c 1 gitflic.ru > /dev/null 2>&1") == 0;
#endif
    }
    
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    static bool downloadFile(const std::string& url, const std::string& outputPath) {
#ifdef _WIN32
        // Реализация для Windows с использованием WinINet
        HINTERNET hInternet = InternetOpenA("EduFlowServer", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return false;
        
        HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hUrl) {
            InternetCloseHandle(hInternet);
            return false;
        }
        
        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open()) {
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            return false;
        }
        
        char buffer[4096];
        DWORD bytesRead;
        bool success = true;
        
        while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            file.write(buffer, bytesRead);
        }
        
        file.close();
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return success;
#else
        // Реализация для Linux с использованием libcurl
        CURL* curl;
        CURLcode res;
        std::string response;
        
        curl = curl_easy_init();
        if (!curl) return false;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "EduFlowServer/1.0");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            return false;
        }
        
        // Сохраняем полученные данные в файл
        std::ofstream file(outputPath);
        if (!file.is_open()) return false;
        
        file << response;
        file.close();
        
        return true;
#endif
    }
};

#endif