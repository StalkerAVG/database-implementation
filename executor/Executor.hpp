#pragma once
#include "../parser/AST.hpp"
#include "../storage-engine/storage_engine.hpp"
#include "../storage-engine/table_manager.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <variant>

inline ColumnType astToStorageType(DataType astType) {
    switch (astType) {
    case DataType::SMALLINT: return SMALLINT;
    case DataType::BIGINT: return BIGINT;
    case DataType::FLOAT: return FLOAT;
    case DataType::SMALLTEXT: return SMALLTEXT;
    case DataType::BIGTEXT: return BIGTEXT;
    case DataType::ID: return ID;
    default: throw std::runtime_error("Unknown data type");
    }
}

inline std::string extractLiteralValue(Expression* expr) {
    if (auto lit = dynamic_cast<Literal*>(expr)) {
        return lit->value;
    }
    throw std::runtime_error("Expected literal expression");
}

class Executor {
private:
    StorageEngine strgeng;
    TableManager tblmgr;
    std::string m_currentDatabase;

    void ensureDatabaseSelected() {
        if (m_currentDatabase.empty()) {
            throw std::runtime_error("No database selected. Use 'USE <data base name>'");
        }
	}

    

public:
    Executor() = default;

    void executeStatement(Statement* stmt) {

        if (auto createDb = dynamic_cast<CreateDatabaseStatement*>(stmt)) {
            strgeng.create_db(createDb->databaseName);
        }

        else if (auto dropDb = dynamic_cast<DropDatabaseStatement*>(stmt)) {
			throw std::logic_error("Drop database not implemented yet.");
        }

        else if (auto useDb = dynamic_cast<UseDatabaseStatement*>(stmt)) {
            m_currentDatabase = useDb->databaseName;
        }
        
        else if (auto createTable = dynamic_cast<CreateTableStatement*>(stmt)) {
            ensureDatabaseSelected();

            std::vector<Column> columns;
            for (const auto& [colName, colType] : createTable->columnNamesWithTypes) {
                ColumnType storageType = astToStorageType(colType);
                bool isUnique = (storageType == ID);
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
                Record record = tblmgr.retrieve_record(m_currentDatabase, selectStmt->tableName, std::stoi(rightLit->value));
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