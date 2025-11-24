// ArticleEditor.cpp
#include "article/ArticleEditor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include "json.hpp"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using json = nlohmann::json;

// Цветовые коды для консоли
namespace ArticleColors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
}

// Вспомогательные функции для UTF-8
int utf8_char_length(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    if (uc < 0x80) return 1;
    if (uc >= 0xC0 && uc < 0xE0) return 2;
    if (uc >= 0xE0 && uc < 0xF0) return 3;
    if (uc >= 0xF0 && uc < 0xF8) return 4;
    return 1; // Ошибка, но пропускаем
}

int utf8_char_count(const std::string& s) {
    int count = 0;
    size_t i = 0;
    while (i < s.length()) {
        int len = utf8_char_length(s[i]);
        i += len;
        count++;
    }
    return count;
}

size_t utf8_char_to_byte(const std::string& s, int char_pos) {
    size_t byte_pos = 0;
    int current_char = 0;
    while (current_char < char_pos && byte_pos < s.length()) {
        int len = utf8_char_length(s[byte_pos]);
        byte_pos += len;
        current_char++;
    }
    return byte_pos;
}

ArticleEditor::ArticleEditor() : newsDirectory("news"), hasUnsavedChanges(false), currentFilename("") {
    std::filesystem::create_directories(newsDirectory);
    
    // Настройка кодировки для Windows
#ifdef _WIN32
    SetConsoleOutputCP(65001); // UTF-8
    SetConsoleCP(65001);
#endif
}

std::string ArticleEditor::getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%d.%m.%Y");
    return ss.str();
}

// В методе parseMarkdown, замените обработку жирного и курсива:
void ArticleEditor::parseMarkdown(const std::string& markdownText) {
    currentArticle.content.clear();
    std::istringstream stream(markdownText);
    std::string line;
    
    while (std::getline(stream, line)) {
        ArticleParagraph paragraph;
        
        // Parse alignment
        if (line.length() >= 2 && line.substr(0, 2) == "<-") {
            paragraph.alignment = "left";
            paragraph.text = line.substr(2);
        } else if (line.length() >= 2 && line.substr(0, 2) == "->") {
            paragraph.alignment = "right";
            paragraph.text = line.substr(2);
        } else if (line.length() >= 2 && line.substr(0, 2) == "<>") {
            paragraph.alignment = "justify";
            paragraph.text = line.substr(2);
        } else if (line.length() >= 2 && line.substr(0, 2) == "^^") {
            paragraph.alignment = "center";
            paragraph.text = line.substr(2);
        } else {
            paragraph.alignment = "justify";
            paragraph.text = line;
        }
        
        // Parse headers
        std::string& text = paragraph.text;
        
        // Убираем пробелы после решеток для корректного определения
        std::string cleanText = text;
        if (cleanText.find("###") == 0) {
            cleanText = cleanText.substr(3);
            //
            if (!cleanText.empty() && cleanText[0] == ' ') {
                cleanText = cleanText.substr(1);
            }
            int char_len = utf8_char_count(cleanText);
            paragraph.formats.push_back({"header1", 0, char_len});
            text = cleanText;
        } else if (cleanText.find("##") == 0) {
            cleanText = cleanText.substr(2);
            // Убираем возможный пробел после ##
            if (!cleanText.empty() && cleanText[0] == ' ') {
                cleanText = cleanText.substr(1);
            }
            int char_len = utf8_char_count(cleanText);
            paragraph.formats.push_back({"header2", 0, char_len});
            text = cleanText;
        } else if (cleanText.find("#") == 0) {
            cleanText = cleanText.substr(1);
            // Убираем возможный пробел после #
            if (!cleanText.empty() && cleanText[0] == ' ') {
                cleanText = cleanText.substr(1);
            }
            int char_len = utf8_char_count(cleanText);
            paragraph.formats.push_back({"header3", 0, char_len});
            text = cleanText;
        }
        
        // Теперь парсим остальное форматирование (жирный, курсив)
        std::vector<std::pair<int, std::string>> openFormats;
        std::string finalText = "";
        std::vector<ArticleFormat> newFormats;
        
        int cleanIndex = 0;
        size_t i = 0;
        
        while (i < text.length()) {
            bool markerFound = false;
            
            // Проверяем маркеры форматирования
            if (i + 3 <= text.length() && text.substr(i, 3) == "***") {
                if (!openFormats.empty() && openFormats.back().second == "***") {
                    int startPos = openFormats.back().first;
                    openFormats.pop_back();
                    newFormats.push_back({"bold_italic", startPos, cleanIndex});
                } else {
                    openFormats.push_back({cleanIndex, "***"});
                }
                i += 3;
                markerFound = true;
            } 
            else if (i + 2 <= text.length() && text.substr(i, 2) == "**") {
                if (!openFormats.empty() && openFormats.back().second == "**") {
                    int startPos = openFormats.back().first;
                    openFormats.pop_back();
                    newFormats.push_back({"bold", startPos, cleanIndex});
                } else {
                    openFormats.push_back({cleanIndex, "**"});
                }
                i += 2;
                markerFound = true;
            }
            else if (i + 2 <= text.length() && text.substr(i, 2) == "__") {
                if (!openFormats.empty() && openFormats.back().second == "__") {
                    int startPos = openFormats.back().first;
                    openFormats.pop_back();
                    newFormats.push_back({"italic", startPos, cleanIndex});
                } else {
                    openFormats.push_back({cleanIndex, "__"});
                }
                i += 2;
                markerFound = true;
            }
            
            if (!markerFound) {
                int len = utf8_char_length(text[i]);
                finalText += text.substr(i, len);
                cleanIndex++;
                i += len;
            }
        }
        
        // Добавляем новые форматы к существующим (к заголовкам)
        text = finalText;
        for (const auto& fmt : newFormats) {
            paragraph.formats.push_back(fmt);
        }
        
        currentArticle.content.push_back(paragraph);
    }
}

std::string ArticleEditor::generateMarkdown() {
    std::stringstream ss;
    
    for (const auto& paragraph : currentArticle.content) {
        std::string line;
        
        // Add alignment prefix
        if (paragraph.alignment == "left") ss << "<-";
        else if (paragraph.alignment == "right") ss << "->";
        else if (paragraph.alignment == "center") ss << "^^";
        else if (paragraph.alignment == "justify") ss << "<>";
        
        std::string text = paragraph.text;
        
        // Обрабатываем заголовки - ИЩЕМ ИХ СРЕДИ ФОРМАТОВ
        std::string headerPrefix = "";
        bool hasHeader = false;
        
        for (const auto& format : paragraph.formats) {
            if (format.type == "header1") {
                hasHeader = true;
                headerPrefix = "###";
                break;
            } else if (format.type == "header2") {
                hasHeader = true;
                headerPrefix = "##";
                break;
            } else if (format.type == "header3") {
                hasHeader = true;
                headerPrefix = "#";
                break;
            }
        }
        
        // Если есть заголовок, добавляем префикс и пробел
        if (hasHeader) {
            ss << headerPrefix << " ";
        }
        
        // Применяем остальные форматы в обратном порядке
        std::vector<ArticleFormat> sortedFormats = paragraph.formats;
        std::sort(sortedFormats.begin(), sortedFormats.end(), 
            [](const ArticleFormat& a, const ArticleFormat& b) {
                return a.start > b.start;
            });
        
        for (const auto& format : sortedFormats) {
            // Пропускаем заголовки - мы их уже обработали
            if (format.type.find("header") != std::string::npos) {
                continue;
            }
            
            std::string opening, closing;
            if (format.type == "bold") {
                opening = "**";
                closing = "**";
            } else if (format.type == "italic") {
                opening = "__";
                closing = "__";
            } else if (format.type == "bold_italic") {
                opening = "***";
                closing = "***";
            }
            
            if (!opening.empty()) {
                size_t byte_end = utf8_char_to_byte(text, format.end);
                text.insert(byte_end, closing);
                
                size_t byte_start = utf8_char_to_byte(text, format.start);
                text.insert(byte_start, opening);
            }
        }
        
        ss << text << "\n";
    }
    
    return ss.str();
}

void ArticleEditor::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void ArticleEditor::displayEditorHeader() {
    using namespace ArticleColors;
    
    std::cout << BOLD << MAGENTA << "📝 РЕДАКТОР СТАТЕЙ" << RESET << std::endl;
    std::cout << MAGENTA << "──────────────────────────────────────────────────────────" << RESET << std::endl;
    std::cout << CYAN << "🏷️  Заголовок: " << WHITE << currentArticle.title << RESET << std::endl;
    std::cout << CYAN << "👤 Автор: " << WHITE << currentArticle.author << RESET << std::endl;
    std::cout << CYAN << "📅 Дата: " << WHITE << currentArticle.date << RESET << std::endl;
    if (!currentFilename.empty()) {
        std::cout << CYAN << "📁 Файл: " << WHITE << currentFilename << RESET << std::endl;
    }
    std::cout << MAGENTA << "──────────────────────────────────────────────────────────" << RESET << std::endl;
}

void ArticleEditor::displayEditBuffer() {
    using namespace ArticleColors;
    
    std::cout << CYAN << "\n📄 СОДЕРЖИМОЕ СТАТЬИ:" << RESET << std::endl;
    if (editBuffer.empty()) {
        std::cout << DIM << "Текст статьи отсутствует" << RESET << std::endl;
    } else {
        for (size_t i = 0; i < editBuffer.size(); ++i) {
            std::string line = editBuffer[i];
            std::string alignIndicator = "";
            
            // Определяем индикатор выравнивания
            if (line.find("<-") == 0) {
                alignIndicator = BLUE + "[←] " + RESET;
                line = line.substr(2);
            } else if (line.find("->") == 0) {
                alignIndicator = GREEN + "[→] " + RESET;
                line = line.substr(2);
            } else if (line.find("<>") == 0) {
                alignIndicator = MAGENTA + "[⟷] " + RESET;
                line = line.substr(2);
            } else if (line.find("^^") == 0) {
                alignIndicator = YELLOW + "[↔] " + RESET;
                line = line.substr(2);
            } else {
                alignIndicator = MAGENTA + "[⟷] " + RESET;
            }
            
            std::cout << GREEN << std::right << std::setw(2) << (i + 1) << ": " << RESET 
                      << alignIndicator << WHITE << line << RESET << std::endl;
        }
    }
    std::cout << CYAN << "──────────────────────────────────────────────────────────" << RESET << std::endl;
}

void ArticleEditor::displayStatusLine(const std::string& message) {
    using namespace ArticleColors;
    
    std::cout << std::endl;
    if (!message.empty()) {
        std::cout << YELLOW << "💬 " << message << RESET << std::endl;
    }
    if (hasUnsavedChanges) {
        std::cout << RED << "⚠️  Есть несохраненные изменения!" << RESET << std::endl;
    }
    std::cout << CYAN << "📝 Введите текст или команду " << DIM << "(.help" << CYAN << " для справки):" << RESET << std::endl;
}

void ArticleEditor::displayStatusLine() {
    displayStatusLine("");
}

std::string ArticleEditor::getInputWithPrompt(const std::string& prompt) {
    using namespace ArticleColors;
    std::cout << YELLOW << prompt << RESET;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

bool ArticleEditor::confirmAction(const std::string& message) {
    using namespace ArticleColors;
    std::cout << YELLOW << "❓ " << message << " (y/n): " << RESET;
    std::string answer;
    std::getline(std::cin, answer);
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
}

void ArticleEditor::loadToEditBuffer() {
    editBuffer.clear();
    for (const auto& paragraph : currentArticle.content) {
        std::string line;
        
        // Добавляем префикс выравнивания
        if (paragraph.alignment == "left") line = "<-";
        else if (paragraph.alignment == "right") line = "->";
        else if (paragraph.alignment == "center") line = "^^";
        else if (paragraph.alignment == "justify") line = "<>";
        
        std::string text = paragraph.text;
        
        // Обрабатываем заголовки
        std::string headerPrefix = "";
        for (const auto& format : paragraph.formats) {
            if (format.type == "header1") {
                headerPrefix = "###";
                break;
            } else if (format.type == "header2") {
                headerPrefix = "##";
                break;
            } else if (format.type == "header3") {
                headerPrefix = "#";
                break;
            }
        }
        
        // Добавляем префикс заголовка если есть
        if (!headerPrefix.empty()) {
            line += headerPrefix + " ";
        }
        
        // Восстанавливаем остальное форматирование
        std::vector<ArticleFormat> sortedFormats = paragraph.formats;
        std::sort(sortedFormats.begin(), sortedFormats.end(), 
            [](const ArticleFormat& a, const ArticleFormat& b) {
                return a.start > b.start;
            });
        
        for (const auto& format : sortedFormats) {
            // Пропускаем заголовки
            if (format.type.find("header") != std::string::npos) {
                continue;
            }
            
            std::string opening, closing;
            if (format.type == "bold") {
                opening = "**";
                closing = "**";
            } else if (format.type == "italic") {
                opening = "__";
                closing = "__";
            } else if (format.type == "bold_italic") {
                opening = "***";
                closing = "***";
            }
            
            if (!opening.empty()) {
                size_t byte_end = utf8_char_to_byte(text, format.end);
                text.insert(byte_end, closing);
                
                size_t byte_start = utf8_char_to_byte(text, format.start);
                text.insert(byte_start, opening);
            }
        }
        
        line += text;
        editBuffer.push_back(line);
    }
}

void ArticleEditor::saveFromEditBuffer() {
    std::string content;
    for (const auto& line : editBuffer) {
        content += line + "\n";
    }
    parseMarkdown(content);
}

void ArticleEditor::waitForEnter() {
    using namespace ArticleColors;
    std::cout << YELLOW << "\n↵ Нажмите Enter для продолжения..." << RESET;
    std::cin.get();
}

bool ArticleEditor::createNewArticle() {
    currentArticle = Article();
    currentArticle.title = "Новая статья";
    currentArticle.author = "Автор";
    currentArticle.date = getCurrentDate();
    currentFilename = "";
    editBuffer.clear();
    hasUnsavedChanges = true;
    return true;
}

void ArticleEditor::editArticle() {
    run();
}

bool ArticleEditor::editArticle(const std::string& filename) {
    if (!filename.empty() && !loadArticle(filename)) {
        return false;
    }
    
    loadToEditBuffer();
    return run();
}

bool ArticleEditor::loadArticle(const std::string& filename) {
    std::ifstream file(newsDirectory + "/" + filename);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        json j;
        file >> j;
        
        currentArticle.title = j["title"];
        currentArticle.author = j["author"];
        currentArticle.date = j["date"];
        currentArticle.content.clear();
        
        for (const auto& item : j["content"]) {
            ArticleParagraph paragraph;
            paragraph.text = item["text"];
            paragraph.alignment = item["alignment"];
            
            for (const auto& fmt : item["formats"]) {
                ArticleFormat format;
                format.type = fmt["type"];
                format.start = fmt["start"];
                format.end = fmt["end"];
                paragraph.formats.push_back(format);
            }
            
            currentArticle.content.push_back(paragraph);
        }
        
        currentFilename = filename;
        hasUnsavedChanges = false;
        return true;
    } catch (...) {
        return false;
    }
}

bool ArticleEditor::saveArticle() {
    if (currentArticle.title.empty()) {
        currentArticle.title = "Без названия";
    }
    
    // Generate safe filename from title
    std::string newFilename = currentArticle.title;
    std::replace(newFilename.begin(), newFilename.end(), ' ', '_');
    std::replace(newFilename.begin(), newFilename.end(), '/', '_');
    std::replace(newFilename.begin(), newFilename.end(), '\\', '_');
    newFilename = newsDirectory + "/" + newFilename + ".json";
    
    // Если файл уже существует и мы меняем название, удаляем старый файл
    if (!currentFilename.empty() && currentFilename != newFilename) {
        std::string oldFilepath = newsDirectory + "/" + currentFilename;
        if (std::filesystem::exists(oldFilepath)) {
            std::filesystem::remove(oldFilepath);
        }
    }
    
    saveFromEditBuffer();
    
    json j;
    j["title"] = currentArticle.title;
    j["author"] = currentArticle.author;
    j["date"] = currentArticle.date;
    j["content"] = json::array();
    
    for (const auto& paragraph : currentArticle.content) {
        json p;
        p["text"] = paragraph.text;
        p["alignment"] = paragraph.alignment;
        p["formats"] = json::array();
        
        for (const auto& format : paragraph.formats) {
            json f;
            f["type"] = format.type;
            f["start"] = format.start;
            f["end"] = format.end;
            p["formats"].push_back(f);
        }
        
        j["content"].push_back(p);
    }
    
    std::ofstream file(newFilename);
    if (!file.is_open()) {
        return false;
    }
    
    file << j.dump(4);
    file.close();
    
    // Обновляем текущее имя файла
    currentFilename = newFilename.substr(newsDirectory.length() + 1);
    hasUnsavedChanges = false;
    return true;
}

void ArticleEditor::listArticles() {
    using namespace ArticleColors;
    
    std::vector<std::string> articles = getArticleFilenames();
    
    std::cout << BOLD << MAGENTA << "\n📂 СПИСОК СТАТЕЙ:" << RESET << std::endl;
    if (articles.empty()) {
        std::cout << DIM << "Статьи не найдены" << RESET << std::endl;
    } else {
        for (size_t i = 0; i < articles.size(); ++i) {
            // Убираем расширение .json для красивого отображения
            std::string articleName = articles[i];
            if (articleName.length() > 5 && articleName.substr(articleName.length() - 5) == ".json") {
                articleName = articleName.substr(0, articleName.length() - 5);
            }
            std::replace(articleName.begin(), articleName.end(), '_', ' ');
            
            std::cout << CYAN << " " << GREEN << std::right << std::setw(2) << (i + 1) << ". " 
                      << WHITE << articleName << RESET << std::endl;
        }
    }
    std::cout << MAGENTA << "──────────────────────────────────────────────────────────" << RESET << std::endl;
}

std::vector<std::string> ArticleEditor::getArticleFilenames() {
    std::vector<std::string> files;
    
    for (const auto& entry : std::filesystem::directory_iterator(newsDirectory)) {
        if (entry.path().extension() == ".json") {
            files.push_back(entry.path().filename().string());
        }
    }
    
    return files;
}

bool ArticleEditor::editLine(int lineNumber, const std::string& newText) {
    if (lineNumber < 1 || lineNumber > static_cast<int>(editBuffer.size())) {
        return false;
    }
    
    std::string oldLine = editBuffer[lineNumber - 1];
    
    // Определяем выравнивание из старой строки
    std::string alignmentPrefix = "";
    if (oldLine.find("<-") == 0) {
        alignmentPrefix = "<-";
    } else if (oldLine.find("->") == 0) {
        alignmentPrefix = "->";
    } else if (oldLine.find("<>") == 0) {
        alignmentPrefix = "<>"; 
    } else if (oldLine.find("^^") == 0) {
        alignmentPrefix = "^^";
    }
    
    // Определяем, есть ли в новой строке собственное выравнивание
    std::string newAlignmentPrefix = "";
    std::string cleanNewText = newText;
    if (newText.find("<-") == 0) {
        newAlignmentPrefix = "<-";
        cleanNewText = newText.substr(2);
    } else if (newText.find("->") == 0) {
        newAlignmentPrefix = "->";
        cleanNewText = newText.substr(2);
    } else if (newText.find("<>") == 0) {
        newAlignmentPrefix = "<>";
        cleanNewText = newText.substr(2);
    } else if (newText.find("^^") == 0) {
        newAlignmentPrefix = "^^";
        cleanNewText = newText.substr(2);
    }
    
    // Формируем финальную строку с учетом выравнивания
    std::string finalText;
    if (!newAlignmentPrefix.empty()) {
        finalText = newAlignmentPrefix + cleanNewText;
    } else {
        finalText = alignmentPrefix + newText;
    }
    
    editBuffer[lineNumber - 1] = finalText;
    hasUnsavedChanges = true;
    return true;
}

bool ArticleEditor::deleteLine(int lineNumber) {
    if (lineNumber < 1 || lineNumber > static_cast<int>(editBuffer.size())) {
        return false;
    }
    
    editBuffer.erase(editBuffer.begin() + (lineNumber - 1));
    hasUnsavedChanges = true;
    return true;
}

void ArticleEditor::showHelp() {
    using namespace ArticleColors;
    
    std::cout << BOLD << MAGENTA << "\n📖 СПРАВКА ПО РЕДАКТОРУ:" << RESET << std::endl;
    std::cout << MAGENTA << "──────────────────────────────────────────────────────────" << RESET << std::endl;
    std::cout << BOLD << WHITE << "КОМАНДЫ РЕДАКТОРА:" << RESET << std::endl;
    std::cout << GREEN << ".title Текст" << WHITE << "    - Установить заголовок статьи" << RESET << std::endl;
    std::cout << GREEN << ".author Текст" << WHITE << "   - Установить автора" << RESET << std::endl;
    std::cout << GREEN << ".date Текст" << WHITE << "     - Установить дату" << RESET << std::endl;
    std::cout << GREEN << ".preview" << WHITE << "        - Предпросмотр статьи" << RESET << std::endl;
    std::cout << GREEN << ".save" << WHITE << "           - Сохранить статью" << RESET << std::endl;
    std::cout << GREEN << ".help" << WHITE << "           - Показать справку" << RESET << std::endl;
    std::cout << GREEN << ".end/.exit" << WHITE << "      - Завершить редактирование" << RESET << std::endl;
    std::cout << GREEN << ".edit Н Текст" << WHITE << "   - Изменить строку с номером Н" << RESET << std::endl;
    std::cout << GREEN << ".delete Н" << WHITE << "       - Удалить строку с номером Н" << RESET << std::endl;
    
    std::cout << BOLD << WHITE << "\nФОРМАТИРОВАНИЕ ТЕКСТА:" << RESET << std::endl;
    std::cout << YELLOW << "###Заголовок" << WHITE << "    - Заголовок 1 уровня" << RESET << std::endl;
    std::cout << YELLOW << "##Заголовок" << WHITE << "      - Заголовок 2 уровня" << RESET << std::endl;
    std::cout << YELLOW << "#Заголовок" << WHITE << "        - Заголовок 3 уровня" << RESET << std::endl;
    std::cout << YELLOW << "**жирный текст**" << WHITE << "   - Жирный текст" << RESET << std::endl;
    std::cout << YELLOW << "__курсивный текст__" << WHITE << " - Курсивный текст" << RESET << std::endl;
    
    std::cout << BOLD << WHITE << "\nВЫРАВНИВАНИЕ (в начале строки):" << RESET << std::endl;
    std::cout << BLUE << "<-Текст" << WHITE << " - Выравнивание по левому краю" << RESET << std::endl;
    std::cout << GREEN << "->Текст" << WHITE << " - Выравнивание по правому краю" << RESET << std::endl;
    std::cout << MAGENTA << "<>Текст" << WHITE << " - Выравнивание по ширине" << RESET << std::endl;
    std::cout << YELLOW << "^^Текст" << WHITE << " - Выравнивание по центру" << RESET << std::endl;
    
    std::cout << BOLD << WHITE << "\nЦВЕТОВОЙ СПРАВОЧНИК:" << RESET << std::endl;
    std::cout << BLUE << "[←]" << WHITE << " - По левому краю  " << GREEN << "[→]" << WHITE << " - По правому краю" << RESET << std::endl;
    std::cout << YELLOW << "[↔]" << WHITE << " - По центру      " << MAGENTA << "[⟷]" << WHITE << " - По ширине" << RESET << std::endl;
    std::cout << MAGENTA << "──────────────────────────────────────────────────────────" << RESET << std::endl;
}

bool ArticleEditor::run() {
    using namespace ArticleColors;
    
    bool editing = true;
    
    while (editing) {
        clearScreen();
        displayEditorHeader();
        displayEditBuffer();
        displayStatusLine();
        
        std::string input;
        
        // Улучшенный ввод для поддержки русского языка на Windows
#ifdef _WIN32
        std::cout << YELLOW << "> " << RESET;
        std::wstring winput;
        std::getline(std::wcin, winput);
        
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, winput.c_str(), (int)winput.size(), NULL, 0, NULL, NULL);
        input = std::string(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, winput.c_str(), (int)winput.size(), &input[0], size_needed, NULL, NULL);
#else
        std::cout << YELLOW << "> " << RESET;
        std::getline(std::cin, input);
#endif
        
        if (input.empty()) {
            continue;
        }
        
        if (input[0] == '.') {
            // Command processing
            if (input == ".end" || input == ".exit") {
                if (hasUnsavedChanges) {
                    if (confirmAction("У вас есть несохраненные изменения. Выйти без сохранения?")) {
                        editing = false;
                    }
                } else {
                    editing = false;
                }
            } else if (input == ".save") {
                if (saveArticle()) {
                    displayStatusLine("✅ Статья сохранена успешно!");
                } else {
                    displayStatusLine("❌ Ошибка сохранения статьи!");
                }
                waitForEnter();
            } else if (input == ".preview") {
                saveFromEditBuffer();
                clearScreen();
                std::cout << BOLD << MAGENTA << "👁️  ПРЕДПРОСМОТР СТАТЬИ" << RESET << std::endl;
                std::cout << MAGENTA << "──────────────────────────────────────────────────────────" << RESET << std::endl;
                std::cout << CYAN << "🏷️  Заголовок: " << WHITE << currentArticle.title << RESET << std::endl;
                std::cout << CYAN << "👤 Автор: " << WHITE << currentArticle.author << RESET << std::endl;
                std::cout << CYAN << "📅 Дата: " << WHITE << currentArticle.date << RESET << std::endl;
                std::cout << MAGENTA << "──────────────────────────────────────────────────────────" << RESET << std::endl;
                
                for (const auto& paragraph : currentArticle.content) {
                    std::string alignIndicator;
                    if (paragraph.alignment == "left") alignIndicator = BLUE + "[←] " + RESET;
                    else if (paragraph.alignment == "right") alignIndicator = GREEN + "[→] " + RESET;
                    else if (paragraph.alignment == "center") alignIndicator = YELLOW + "[↔] " + RESET;
                    else alignIndicator = MAGENTA + "[⟷] " + RESET;
                    
                    // Apply formatting for preview
                    std::string text = paragraph.text;
                    for (const auto& format : paragraph.formats) {
                        if (format.type == "header1") {
                            text = BOLD + MAGENTA + text + RESET;
                        } else if (format.type == "header2") {
                            text = BOLD + CYAN + text + RESET;
                        } else if (format.type == "header3") {
                            text = BOLD + YELLOW + text + RESET;
                        } else if (format.type == "bold") {
                            text = BOLD + text + RESET;
                        } else if (format.type == "italic") {
                            text = DIM + text + RESET;
                        }
                    }
                    
                    std::cout << alignIndicator << WHITE << text << RESET << std::endl;
                }
                std::cout << MAGENTA << "──────────────────────────────────────────────────────────" << RESET << std::endl;
                waitForEnter();
            } else if (input == ".help") {
                showHelp();
                waitForEnter();
            } else if (input.find(".title ") == 0) {
                currentArticle.title = input.substr(7);
                hasUnsavedChanges = true;
                displayStatusLine("✅ Заголовок обновлен!");
                waitForEnter();
            } else if (input.find(".author ") == 0) {
                currentArticle.author = input.substr(8);
                hasUnsavedChanges = true;
                displayStatusLine("✅ Автор обновлен!");
                waitForEnter();
            } else if (input.find(".date ") == 0) {
                currentArticle.date = input.substr(6);
                hasUnsavedChanges = true;
                displayStatusLine("✅ Дата обновлена!");
                waitForEnter();
            } else if (input.find(".edit ") == 0) {
                std::string rest = input.substr(6);
                size_t spacePos = rest.find(' ');
                if (spacePos != std::string::npos) {
                    try {
                        int lineNum = std::stoi(rest.substr(0, spacePos));
                        std::string newText = rest.substr(spacePos + 1);
                        
                        if (editLine(lineNum, newText)) {
                            displayStatusLine("✅ Строка " + std::to_string(lineNum) + " успешно изменена!");
                        } else {
                            displayStatusLine("❌ Ошибка: неверный номер строки!");
                        }
                    } catch (...) {
                        displayStatusLine("❌ Ошибка: неверный формат команды!");
                    }
                } else {
                    displayStatusLine("❌ Ошибка: неверный формат команды! Используйте: .edit Н Текст");
                }
                waitForEnter();
            } else if (input.find(".delete ") == 0) {
                std::string rest = input.substr(8);
                try {
                    int lineNum = std::stoi(rest);
                    
                    if (deleteLine(lineNum)) {
                        displayStatusLine("✅ Строка " + std::to_string(lineNum) + " успешно удалена!");
                    } else {
                        displayStatusLine("❌ Ошибка: неверный номер строки!");
                    }
                } catch (...) {
                    displayStatusLine("❌ Ошибка: неверный формат команды! Используйте: .delete Н");
                }
                waitForEnter();
            } else {
                displayStatusLine("❌ Неизвестная команда. Введите .help для справки.");
                waitForEnter();
            }
        } else {
            // Text input
            editBuffer.push_back(input);
            hasUnsavedChanges = true;
        }
    }
    
    return true;
}