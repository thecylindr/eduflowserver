#include "database/DatabaseService.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>

// Portfolio management
std::vector<StudentPortfolio> DatabaseService::getPortfolios() {
    std::vector<StudentPortfolio> portfolios;
    
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return portfolios;
    }
    
    std::string sql = R"(
        SELECT sp.portfolio_id, sp.student_code, sp.measure_code, sp.date, sp.decree,
            s.last_name, s.first_name, s.middle_name
        FROM student_portfolio sp
        LEFT JOIN students s ON sp.student_code = s.student_code
        ORDER BY sp.date DESC
    )";
    
    PGresult* res = PQexec(connection, sql.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка выполнения запроса портфолио: " << PQerrorMessage(connection) << std::endl;
        PQclear(res);
        return portfolios;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        StudentPortfolio portfolio;
        portfolio.portfolioId = std::stoi(PQgetvalue(res, i, 0));
        portfolio.studentCode = std::stoi(PQgetvalue(res, i, 1));
        portfolio.measureCode = std::stoi(PQgetvalue(res, i, 2));
        portfolio.date = PQgetvalue(res, i, 3);
        portfolio.decree = std::stoi(PQgetvalue(res, i, 4));
        
        // Формируем полное имя студента
        std::string lastName = PQgetvalue(res, i, 5);
        std::string firstName = PQgetvalue(res, i, 6);
        std::string middleName = PQgetvalue(res, i, 7);
        portfolio.studentName = lastName + " " + firstName + " " + middleName;
        
        portfolios.push_back(portfolio);
    }
    
    PQclear(res);
    return portfolios;
}

bool DatabaseService::addPortfolio(const StudentPortfolio& portfolio) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    
    std::string sql = R"(
        INSERT INTO student_portfolio 
        (student_code, date, decree) 
        VALUES ($1, $2, $3)
    )";
    
    const char* params[3] = {
        std::to_string(portfolio.studentCode).c_str(),
        portfolio.date.c_str(),
        std::to_string(portfolio.decree).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 3, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (success) {
        std::cout << "✅ Портфолио добавлено" << std::endl;
    } else {
        std::cerr << "Ошибка добавления портфолио: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::updatePortfolio(const StudentPortfolio& portfolio) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = R"(
        UPDATE student_portfolio 
        SET student_code = $1, date = $2, decree = $3
        WHERE portfolio_id = $4
    )";
    
    const char* params[4] = {
        std::to_string(portfolio.studentCode).c_str(),
        portfolio.date.c_str(),
        std::to_string(portfolio.decree).c_str(),
        std::to_string(portfolio.portfolioId).c_str()
    };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 4, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка обновления портфолио: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

bool DatabaseService::deletePortfolio(int portfolioId) {
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return false;
    }
    
    std::string sql = "DELETE FROM student_portfolio WHERE portfolio_id = $1";
    const char* params[1] = { std::to_string(portfolioId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Ошибка удаления портфолио: " << PQerrorMessage(connection) << std::endl;
    }
    
    PQclear(res);
    return success;
}

StudentPortfolio DatabaseService::getPortfolioById(int portfolioId) {
    StudentPortfolio portfolio;
    configManager.loadConfig(currentConfig);
    
    if (!connection && !connect(currentConfig)) {
        return portfolio;
    }
    
    std::string sql = R"(
        SELECT sp.portfolio_id, sp.student_code, sp.measure_code, sp.date, sp.decree,
            s.last_name, s.first_name, s.middle_name
        FROM student_portfolio sp
        LEFT JOIN students s ON sp.student_code = s.student_code
        WHERE sp.portfolio_id = $1
    )";
    
    const char* params[1] = { std::to_string(portfolioId).c_str() };
    
    PGresult* res = PQexecParams(connection, sql.c_str(), 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return portfolio;
    }
    
    portfolio.portfolioId = std::stoi(PQgetvalue(res, 0, 0));
    portfolio.studentCode = std::stoi(PQgetvalue(res, 0, 1));
    portfolio.measureCode = std::stoi(PQgetvalue(res, 0, 2));
    portfolio.date = PQgetvalue(res, 0, 3);
    portfolio.decree = std::stoi(PQgetvalue(res, 0, 4));
    
    // Формируем имя студента
    std::string lastName = PQgetvalue(res, 0, 5);
    std::string firstName = PQgetvalue(res, 0, 6);
    std::string middleName = PQgetvalue(res, 0, 7);
    portfolio.studentName = lastName + " " + firstName + " " + middleName;
    
    PQclear(res);
    return portfolio;
}