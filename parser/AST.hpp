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

class AST{
public:
	virtual ~AST() = default;
};

class Expression : public AST {
public:
	virtual ~Expression() = default;
};

class ColumnReference : public Expression {
public:
    std::string columnName;
	ColumnReference(const std::string& name) : columnName(name) {}
};

class Literal : public Expression {
public:
	std::string value;
	Literal(const std::string& val) : value(val) {}
};

class BinaryExpression : public Expression {
public:
	std::shared_ptr<Expression> left;
	std::shared_ptr<Expression> right;
	std::string op;

	BinaryExpression(std::shared_ptr<Expression> l, std::shared_ptr<Expression> r, const std::string& oper)
		: left(l), right(r), op(oper) {}
};

class Statement : public AST {
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
	std::shared_ptr<Expression> whereClause;
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
	std::shared_ptr<Expression> whereClause;
};