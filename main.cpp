#include <iostream>
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "executor/Executor.hpp"
#include "storage-engine/table_manager.hpp"

void processSQL(Executor& executor, const std::string& sql) {
    try {
        Lexer lexer(sql);
        Parser parser(lexer);
        auto stmt = parser.parseStatement();
        executor.executeStatement(stmt.get());
    } 
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

int main(){
    std::cout << "Integration Test Suite" << std::endl;
    
    Executor executor;

    std::string create_db = "CREATE DATABASE testdb3";
    processSQL(executor, create_db);
    
    std::string use_db = "USE testdb3";
    processSQL(executor, use_db);

    std::string create_table = "CREATE TABLE table1 (col1 ID, col2 BIGTEXT)";
    processSQL(executor, create_table);

    std::string drop_table = "DROP TABLE table3";
    processSQL(executor, drop_table);

    std::string insert_record = "INSERT INTO table1 VALUES (1, \"Who\"), (2, \"is\"), (3, \"Mario?\"), (4, \"Its me Mario\")";
    processSQL(executor, insert_record);

    std::string select = "SELECT * FROM table1  WHERE col1 = 4";
    processSQL(executor, select);

    std::string select2 = "SELECT * FROM table1  WHERE col1 = 3";
    processSQL(executor, select2);

    std::string delete_record = "DELETE FROM table1 WHERE col1 = 4";
    processSQL(executor, delete_record);

    processSQL(executor, select);
    processSQL(executor, select2);
    
    return 0;
}