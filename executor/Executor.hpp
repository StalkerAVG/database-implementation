#pragma once
#include "../parser/AST.hpp"
#include "../storage-engine/storage_engine.hpp"
#include "../storage-engine/table_manager.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <variant>

namespace fs = std::filesystem;

inline ak::ColumnType astToStorageType(amk::DataType astType) {
    /**
     * @brief Convert AST DataType to StorageEngine ColumnType 
     * @param astType AST DataType
     * @return Corresponding StorageEngine ColumnType
     */
    switch (astType) {
    case amk::DataType::SMALLINT: return ak::SMALLINT;
    case amk::DataType::BIGINT: return ak::BIGINT;
    case amk::DataType::FLOAT: return ak::FLOAT;
    case amk::DataType::SMALLTEXT: return ak::SMALLTEXT;
    case amk::DataType::BIGTEXT: return ak::BIGTEXT;
    case amk::DataType::ID: return ak::ID;
    default: throw std::runtime_error("Unknown data type");
    }
}

inline std::string extractLiteralValue(amk::Expression* expr) {
    /**
     * @brief Extract string value from Literal expression
     * @param expr Expression pointer
     * @return String value of the literal
     */
    if (auto lit = dynamic_cast<amk::Literal*>(expr)) {
        return lit->value;
    }
    throw std::runtime_error("Expected literal expression");
}

namespace amk {

class Executor {
private:
    ak::StorageEngine strgeng;
    ak::TableManager tblmgr;
    std::string m_currentDatabase;

    struct ColumnInfo {
    bool exists;
    bool hasIndex;
    };

    void ensureDatabaseSelected() {
        if (m_currentDatabase.empty()) {
            throw std::runtime_error("No database selected. Use 'USE <data base name>'");
        }
	}

    ColumnInfo checkColumnIndex(const std::string& dbName, 
                                const std::string& tableName, 
                                const std::string& columnName) {
        /** @brief Check if a column exists and has an index
          * @param dbName Database name
          * @param tableName Table name
          * @param columnName Column name
          * @return ColumnInfo struct with existence and index info
        */
        ColumnInfo result = {false, false};

        std::string filePath = strgeng.get_main_directory() + "/" + dbName + "/" + strgeng.get_meta_file();

        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open metadata file: " + filePath);
        }
        
        std::string line;
        bool foundTable = false;
        int columnsToRead = 0;
        int linesSkipped = 0;
        
        while (std::getline(file, line)) {
            if (!foundTable) {
                if (line == tableName) {
                    foundTable = true;
                    linesSkipped = 0;
                }
            } else {
                linesSkipped++;
                
                if (linesSkipped == 2) {
                    columnsToRead = std::stoi(line);
                } else if (linesSkipped > 2 && columnsToRead > 0) {
                    std::stringstream ss(line);
                    std::string colName, indexFlag;
                    
                    if (std::getline(ss, colName, ',') && std::getline(ss, indexFlag, ',')) {
                        if (colName == columnName) {
                            result.exists = true;
                            result.hasIndex = (indexFlag == "1");
                            file.close();
                            return result;
                        }
                    }
                    
                    columnsToRead--;

                    if (columnsToRead == 0) {
                        break;
                    }
                }
            }
        }
        
        file.close();
        return result;
    }


public:
    Executor() = default;

    void executeStatement(Statement* stmt) {
        /** @brief Execute a given SQL statement
         *  @param stmt Pointer to the statement to execute
         */

        if (auto createDb = dynamic_cast<CreateDatabaseStatement*>(stmt)) {
            strgeng.create_db(createDb->databaseName);
        }

        else if (auto dropDb = dynamic_cast<DropDatabaseStatement*>(stmt)) {
            strgeng.drop_db(dropDb->databaseName);
        }

        else if (auto useDb = dynamic_cast<UseDatabaseStatement*>(stmt)) {
            m_currentDatabase = useDb->databaseName;
        }
        
        else if (auto createTable = dynamic_cast<CreateTableStatement*>(stmt)) {
            ensureDatabaseSelected();

            std::vector<ak::Column> columns;
            for (const auto& [colName, colType] : createTable->columnNamesWithTypes) {
                ak::ColumnType storageType = astToStorageType(colType);
                bool isUnique = (storageType == ak::ID);
                columns.emplace_back(colName, storageType, isUnique);
            }

            strgeng.create_table(m_currentDatabase, createTable->tableName, columns);
        }

        else if (auto dropTable = dynamic_cast<DropTableStatement*>(stmt)) {
            ensureDatabaseSelected();
            
            strgeng.drop_table(m_currentDatabase, dropTable->tableName);
        }

        else if (auto insertStmt = dynamic_cast<InsertStatement*>(stmt)) {
            ensureDatabaseSelected();

            for (const auto& rowValues : insertStmt->values) {
                std::vector<std::string> valuesStr;
                for (const auto& expr : rowValues) {
                    valuesStr.push_back(extractLiteralValue(expr.get()));
                }
                
                tblmgr.add_record(m_currentDatabase, insertStmt->tableName, valuesStr);
            }
        }

        else if (auto selectStmt = dynamic_cast<SelectStatement*>(stmt)) {
            ensureDatabaseSelected();
            if (dynamic_cast<Literal*>(selectStmt->columns[0].get())->value != "*") {
                throw std::runtime_error("Only SELECT * is supported currently.");
            }

            auto binaryExpr = dynamic_cast<BinaryExpression*>(selectStmt->whereClause.get());
            if (!binaryExpr) {
                throw std::runtime_error("Only simple binary expressions are supported in WHERE clause.");
            }

            auto leftColRef = dynamic_cast<ColumnReference*>(binaryExpr->left.get());
            auto rightLit  = dynamic_cast<Literal*>(binaryExpr->right.get());
            
            if(binaryExpr->op == "=" && leftColRef && rightLit) {
                if(!checkColumnIndex(m_currentDatabase, selectStmt->tableName, leftColRef->columnName).exists || 
                   !checkColumnIndex(m_currentDatabase, selectStmt->tableName, leftColRef->columnName).hasIndex) {
                    throw std::runtime_error("This is not an index column.");
                }
                ak::Record record = tblmgr.retrieve_record(m_currentDatabase, selectStmt->tableName, std::stoi(rightLit->value));
                std::cout << record << std::endl;
            }

            else if (binaryExpr->op == "<" && leftColRef && rightLit) {
                throw std::logic_error("Select with '<' not implemented yet.");
            }

            else if (binaryExpr->op == ">" && leftColRef && rightLit) {
                throw std::logic_error("Select with '>' not implemented yet.");
            }
            
            else {
                throw std::runtime_error("Unsupported WHERE clause operation or operands.");
            }

		}

        else if (auto deleteStmt = dynamic_cast<DeleteStatement*>(stmt)) {
            ensureDatabaseSelected();
            
            auto binaryExpr = dynamic_cast<BinaryExpression*>(deleteStmt->whereClause.get());
            if (!binaryExpr) {
                throw std::runtime_error("Only simple binary expressions are supported in WHERE clause.");
            }
            
            auto leftColRef = dynamic_cast<ColumnReference*>(binaryExpr->left.get());
            auto rightLit  = dynamic_cast<Literal*>(binaryExpr->right.get());
            
            if(binaryExpr->op != "=" || !leftColRef || !rightLit) {
                throw std::runtime_error("Unsupported WHERE clause operation or operands.");
            }

            tblmgr.delete_record(m_currentDatabase, deleteStmt->tableName, std::stoi(rightLit->value));
        }

        else {
            throw std::runtime_error("Unknown statement type");
        }
	}
};

}
