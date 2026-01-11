#pragma once
#include "Lexer.hpp"
#include "AST.hpp"
#include <memory>
#include <vector>
#include <stdexcept>

namespace amk {

class Parser {
private:
	Lexer& m_lexer;
	Token m_previousToken;
	Token m_currentToken;

	void advance() {
		m_currentToken = m_lexer.getNextToken();
	}

	std::unique_ptr<Statement> parseCreateDatabaseStatement() {
		/** @brief Parse a CREATE DATABASE statement
		  * @return Unique pointer to the parsed CreateDatabaseStatement 
		*/
		
		auto stmt = std::make_unique<CreateDatabaseStatement>();
		advance();

		if(m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected database name after CREATE DATABASE");
		}

		stmt->databaseName = m_currentToken.lexeme;
		advance();

		return stmt;
	}

	std::unique_ptr<Statement> parseDropDatabaseStatement() {
		/** @brief Parse a DROP DATABASE statement
		  * @return Unique pointer to the parsed DropDatabaseStatement 
		*/
		
		auto stmt = std::make_unique<DropDatabaseStatement>();
		advance();
		
		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected database name after Drop");
		}
		
		stmt->databaseName = m_currentToken.lexeme;
		advance();
		
		return stmt;
	}

	std::unique_ptr<Statement> parseUseDatabaseStatement() {
		/** @brief Parse a USE DATABASE statement
		  * @return Unique pointer to the parsed UseDatabaseStatement 
		*/
		
		auto stmt = std::make_unique<UseDatabaseStatement>();
		advance();
		
		if(m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected database name after USE");
		}
		
		stmt->databaseName = m_currentToken.lexeme;
		advance();
		
		return stmt;
	}

	std::unique_ptr<Statement> parseCreateTableStatement() {
		/** @brief Parse a CREATE TABLE statement
		  * @return Unique pointer to the parsed CreateTableStatement 
		*/
		
		auto stmt = std::make_unique<CreateTableStatement>();
		advance();
		
		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected table name after CREATE TABLE");
		}
		
		stmt->tableName = m_currentToken.lexeme;
		advance();
		
		if(m_currentToken.type != TokenKind::LEFT_PAREN) {
			throw std::runtime_error("Expected '(' after table name");
		}
		advance();

		while (m_currentToken.type == TokenKind::IDENTIFIER) {

			std::string columnName = m_currentToken.lexeme;
			advance();

			if (m_currentToken.type != TokenKind::DATA_TYPE) {
				throw std::runtime_error("Expected data type after column name");
			}

			DataType columnType = stringToDataType(m_currentToken.lexeme);
			stmt->columnNamesWithTypes.emplace_back(columnName, columnType);
			advance();

			if (m_currentToken.type == TokenKind::COMMA) {
				advance();
			}
			else if (m_currentToken.type == TokenKind::RIGHT_PAREN) {
				break;
			}
			else {
				throw std::runtime_error("Expected ',' or ')' in column list");
			}
		}

		if (stmt->columnNamesWithTypes.empty()) {
			throw std::runtime_error("Table must have at least one column");
		}

		if (m_currentToken.type != TokenKind::RIGHT_PAREN) {
			throw std::runtime_error("Expected ')' after column list");
		}
		advance();
		return stmt;
	}

	std::unique_ptr<Statement> parseDropTableStatement() {
		/** @brief Parse a DROP TABLE statement
		  * @return Unique pointer to the parsed DropTableStatement 
		*/
		
		auto stmt = std::make_unique<DropTableStatement>();
		advance();
		
		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected table name after DROP TABLE");
		}
		
		stmt->tableName = m_currentToken.lexeme;
		advance();
		return stmt;
	}

	std::unique_ptr<Statement> parseSelectStatement() {
		/** @brief Parse a SELECT statement
		  * @return Unique pointer to the parsed SelectStatement 
		*/
		
		auto stmt = std::make_unique<SelectStatement>();
		advance();
		
		if (m_currentToken.type != TokenKind::ASTERISK) {
			throw std::runtime_error("Only SELECT * is supported currently.");
		}
		
		stmt->columns.push_back(std::make_unique<Literal>("*"));
		advance();
		
		if (m_currentToken.lexeme != "FROM") {
			throw std::runtime_error("Expected FROM after column list");
		}
		advance();

		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected table name after FROM");
		}

		stmt->tableName = m_currentToken.lexeme;
		advance();

		if (m_currentToken.lexeme == "WHERE") {
			advance();
			stmt->whereClause = parseBinaryExpression();
		}
		advance();
		return stmt;
	}

	std::unique_ptr<Statement> parseInsertStatement() {
		/** @brief Parse an INSERT statement
		  * @return Unique pointer to the parsed InsertStatement 
		*/
		
		auto stmt = std::make_unique<InsertStatement>();
		advance();

		if (m_currentToken.type != TokenKind::KEYWORD || m_currentToken.lexeme != "INTO") {
			throw std::runtime_error("Expected INTO after INSERT");
		}
		advance();

		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected table name after INTO");
		}
		
		stmt->tableName = m_currentToken.lexeme;
		advance();

		if (m_currentToken.type != TokenKind::KEYWORD || m_currentToken.lexeme != "VALUES") {
			throw std::runtime_error("Expected VALUES after table name");
		}
		advance();

		while (m_currentToken.type == TokenKind::LEFT_PAREN) {
			advance();

			std::vector<std::unique_ptr<Expression>> rowValues;
			while (m_currentToken.type != TokenKind::RIGHT_PAREN) {
				rowValues.push_back(parseLiteralExpression());

				if (m_currentToken.type == TokenKind::COMMA) {
					advance();
				}
				else if (m_currentToken.type != TokenKind::RIGHT_PAREN) {
					throw std::runtime_error("Expected ',' or ')' in VALUES list");
				}
			}

			if (rowValues.empty()) {
				throw std::runtime_error("Empty VALUES clause");
			}

			stmt->values.push_back(std::move(rowValues));
			advance();

			if (m_currentToken.type == TokenKind::COMMA) {
				advance();
			}
			else if (m_currentToken.type == TokenKind::SEMI_COLON ||
				m_currentToken.type == TokenKind::END_OF_FILE) {
				break;
			}
			else {
				throw std::runtime_error("Expected ',' or ';' after VALUES clause");
			}
		}

		return stmt;
	}

	std::unique_ptr<Statement> parseDeleteStatement() {
		/** @brief Parse a DELETE statement
		  * @return Unique pointer to the parsed DeleteStatement 
		*/
		
		auto stmt = std::make_unique<DeleteStatement>();
		advance();
		
		if (m_currentToken.lexeme != "FROM") {
			throw std::runtime_error("Expected FROM after DELETE");
		}
		advance();
		
		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected table name after FROM");
		}
		
		stmt->tableName = m_currentToken.lexeme;
		advance();
		
		if (m_currentToken.lexeme == "WHERE") {
			advance();
			stmt->whereClause = parseBinaryExpression();
		}
		advance();
		
		return stmt;
	}

	std::shared_ptr<Expression> parseColumnReference() {
		/** @brief Parse a column reference expression
		  * @return Shared pointer to the parsed ColumnReference 
		*/
		
		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::logic_error("Expected column reference");
		}
		
		std::string columnName = m_currentToken.lexeme;
		advance();
		return std::make_shared<ColumnReference>(columnName);
	}

	std::unique_ptr<Expression> parseLiteralExpression() {
		/** @brief Parse a literal expression
		  * @return Unique pointer to the parsed Literal 
		*/
		
		if (m_currentToken.type == TokenKind::NUMBER) {
			
			std::string lexeme = m_currentToken.lexeme;
			advance();
			
			if (lexeme.find('.') != std::string::npos) {
				return std::make_unique<Literal>(std::to_string(std::stof(lexeme)));
			}
			else {
				return std::make_unique<Literal>(std::to_string(std::stoi(lexeme)));
			}
		}

		else if (m_currentToken.type == TokenKind::STRING) {
			std::string lexeme = m_currentToken.lexeme;
			advance();
			return std::make_unique<Literal>(lexeme);
		}

		else{
			throw std::logic_error("Expected literal");
		}
		
	}

	std::shared_ptr<Expression> parseBinaryExpression() {
		/** @brief Parse a binary expression
		  * @return Shared pointer to the parsed BinaryExpression 
		*/
		
		auto left = parseColumnReference();

		if (m_currentToken.type != TokenKind::EQUAL &&
			m_currentToken.type != TokenKind::LESS_THAN &&
			m_currentToken.type != TokenKind::GREATER_THAN) {
			
			throw std::runtime_error("Expected binary operator (=, <, >)");
		}
		
		std::string op = m_currentToken.lexeme;
		advance();
		auto right = parseLiteralExpression();
		//pointers issues
		//move() is required for unique_ptr
		return std::make_shared<BinaryExpression>(std::move(left), std::move(right), op);
	}

public:
	/** @brief Constructor for the Parser class
	  * @param lexer Reference to the Lexer instance
	*/
	Parser(Lexer& lexer) : m_lexer(lexer) {
		advance();
	}

	/** @brief Constructor for the Parser class with previous token
	  * @param lexer Reference to the Lexer instance
	  * @param token Reference to the previous Token
	*/
	Parser(Lexer& lexer, Token& token) : m_lexer(lexer), m_previousToken(token) {
		advance();
	}

	std::unique_ptr<Statement> parseStatement() {
		/** @brief Parse a SQL statement
		  * @return Unique pointer to the parsed Statement 
		*/

		if (m_currentToken.type == TokenKind::KEYWORD) {

			if (m_currentToken.lexeme == "SELECT") {
				return parseSelectStatement();
			}

			else if (m_currentToken.lexeme == "INSERT") {
				return parseInsertStatement();
			}

			else if (m_currentToken.lexeme == "DELETE") {
				return parseDeleteStatement();
			}

			else if (m_currentToken.lexeme == "CREATE") {
				m_previousToken = m_currentToken;
				advance();

				if (m_currentToken.lexeme == "DATABASE") {
					return parseCreateDatabaseStatement();
				}
				else if(m_currentToken.lexeme == "TABLE") {
					return parseCreateTableStatement();
				}
				else {
					throw std::runtime_error("Expected DATABASE, COLUMN, TABLE, or USE after CREATE");
				}

			}

			else if (m_currentToken.lexeme == "DROP") {
				m_previousToken = m_currentToken;
				advance();

				if (m_currentToken.lexeme == "TABLE") {
					return parseDropTableStatement();
				}
				else if (m_currentToken.lexeme == "DATABASE") {
					return parseDropDatabaseStatement();
				}
				else {
					throw std::runtime_error("Expected TABLE, COLUMN, or DATABASE after DROP");
				}

			}

			else if (m_currentToken.lexeme == "USE") {
				return parseUseDatabaseStatement();
			}

		}
		
		throw std::runtime_error("Unknown statement type");
	}

	DataType stringToDataType(const std::string& input) {
		/** @brief Convert a string representation of a data type to its enum value
		  * @param input String representation of the data type
		  * @return Corresponding DataType enum value
		*/
		
		auto it = stringToDataTypeMap.find(input);
		if (it != stringToDataTypeMap.end()) {
			return it->second;
		}

		throw std::runtime_error("Unknown DataType: " + input);
	}
};

}