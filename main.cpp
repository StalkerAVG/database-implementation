#include <iostream>
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "executor/Executor.hpp"
#include "storage-engine/table_manager.hpp"

void processSQL(amk::Executor& executor, const std::string& sql) {
    try {
        amk::Lexer lexer(sql);
        amk::Parser parser(lexer);
        auto stmt = parser.parseStatement();
        executor.executeStatement(stmt.get());
    } 
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

int main(){
    std::cout << "Integration Test Suite" << std::endl;
    
    amk::Executor executor;

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

    std::string select3 = "SELECT * FROM table1  WHERE col2 = \"Its me Mario\"";
    processSQL(executor, select3);

    std::string select4 = "SELECT * FROM table1  WHERE col1 > 2";
    processSQL(executor, select4);

    std::string select5 = "SELECT * FROM table1  WHERE col1 < 3";
    processSQL(executor, select5);

    std::string select6 = "SELECT * FROM table1";
    processSQL(executor, select6);

    std::string select7 = "SELECT col1, col2 FROM table1";
    processSQL(executor, select7);

    std::string delete_record = "DELETE FROM table1 WHERE col1 = 4";
    processSQL(executor, delete_record);

    processSQL(executor, select);
    processSQL(executor, select2);


    std::string drop_db = "DROP DATABASE testdb3";
    processSQL(executor, drop_db);
    
    return 0;
}