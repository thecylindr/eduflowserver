#ifndef ARTICLEEDITOR_H
#define ARTICLEEDITOR_H

#include <string>
#include <vector>
#include <chrono>

struct ArticleFormat {
    std::string type;
    int start;
    int end;
};

struct ArticleParagraph {
    std::string text;
    std::string alignment;
    std::vector<ArticleFormat> formats;
};

struct Article {
    std::string title;
    std::string author;
    std::string date;
    std::vector<ArticleParagraph> content;
};

class ArticleEditor {
private:
    std::string newsDirectory;
    Article currentArticle;
    std::vector<std::string> editBuffer;
    bool hasUnsavedChanges;
    std::string currentFilename; // ДОБАВЛЕНО: для отслеживания текущего файла

    std::string getCurrentDate();
    void parseMarkdown(const std::string& markdownText);
    std::string generateMarkdown();
    void clearScreen();
    void displayEditorHeader();
    void displayEditBuffer();
    void displayStatusLine(const std::string& message);
    void displayStatusLine();
    std::string getInputWithPrompt(const std::string& prompt);
    bool confirmAction(const std::string& message);
    void loadToEditBuffer();
    void saveFromEditBuffer();
    void waitForEnter();
    void showHelp();
    bool run();

    // НОВЫЕ ФУНКЦИИ ДЛЯ РЕДАКТИРОВАНИЯ И УДАЛЕНИЯ СТРОК
    bool editLine(int lineNumber, const std::string& newText);
    bool deleteLine(int lineNumber);

public:
    ArticleEditor();
    bool createNewArticle();
    void editArticle();
    bool editArticle(const std::string& filename);
    bool loadArticle(const std::string& filename);
    bool saveArticle();
    void listArticles();
    std::vector<std::string> getArticleFilenames();
};

#endif