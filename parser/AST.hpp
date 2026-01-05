#pragma once
#include <string>
#include <memory>
#include <vector>
#include <variant>

enum class DataType {
	SMALLINT,
	BIGINT,
	FLOAT,
	SMALLTEXT,
	BIGTEXT,
	ID,
};

class Expression {
public:
	virtual ~Expression() = default;
};

// Requires C++17 or higher for std::variant
using LiteralValue = std::variant<int, float, std::string, bool>;

class Literal : public Expression {
public:
	LiteralValue value;
	Literal(const LiteralValue val) : value(val) {}
};

class Statement {
public:
	virtual ~Statement() = default;
};

class CreateDatabaseStatement : public Statement {
public:
	std::string databaseName;
};

class DropDatabaseStatement : public Statement {
public:
	std::string databaseName;
};

class UseDatabaseStatement : public Statement {
public:
	std::string databaseName;
};

class CreateColumnStatement : public Statement {
public:
	std::string columnName;
	DataType columnType;
	std::string databaseName;
};

class DropColumnStatement : public Statement {
public:
	std::string columnName;
	std::string databaseName;
};

class CreateTableStatement : public Statement {
public:
	std::string tableName;
	std::vector<std::pair<std::string, DataType>> columnNamesWithTypes;
	std::string databaseName;
};

class DropTableStatement : public Statement {
public:
	std::string tableName;
	std::string databaseName;
};

class SelectStatement : public Statement {
public:
	std::vector<std::unique_ptr<Expression>> columns;
	std::string tableName;
	std::string databaseName; 
};

class InsertStatement : public Statement {
public:
	std::string databaseName;
    std::string tableName;
    std::vector<std::vector<std::unique_ptr<Expression>>> values;
};

class DeleteStatement : public Statement {
public:
	std::string tableName;
	std::string databaseName;
};