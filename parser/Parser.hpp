#pragma once
#include "Lexer.hpp"
#include "AST.hpp"
#include <memory>
#include <vector>
#include <stdexcept>

class Parser {
private:
	Lexer& m_lexer;
	Token m_previousToken;
	Token m_currentToken;

	void advance() {
		m_currentToken = m_lexer.getNextToken();
	}

	std::unique_ptr<Statement> parseCreateDatabaseStatement() {
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
		auto stmt = std::make_unique<UseDatabaseStatement>();
		advance();
		if(m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected database name after USE");
		}
		stmt->databaseName = m_currentToken.lexeme;
		advance();
		
		return stmt;
	}

	std::unique_ptr<Statement> parseCreateColumnStatement() {
		auto stmt = std::make_unique<CreateColumnStatement>();
		advance();
		if(m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected column name after CREATE COLUMN");
		}
		stmt->columnName = m_currentToken.lexeme;
		advance();
		if(m_currentToken.type != TokenKind::DATA_TYPE) {
			throw std::runtime_error("Expected data type after column name");
		}
		stmt->columnType = stringToDataType(m_currentToken.lexeme);
		advance();
		
		return stmt;
	}

	std::unique_ptr<Statement> parseDropColumnStatement() {
		auto stmt = std::make_unique<DropColumnStatement>();
		advance();
		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected column name after DROP COLUMN");
		}
		stmt->columnName = m_currentToken.lexeme;
		advance();
		return stmt;
	}

	std::unique_ptr<Statement> parseCreateTableStatement() {
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
		auto stmt = std::make_unique<SelectStatement>();
		advance();
		if (m_currentToken.type == TokenKind::ASTERISK) {
			stmt->columns.push_back(std::make_unique<Literal>(LiteralValue{ "*"}));
			advance();
		}
		else {
			while (m_currentToken.type == TokenKind::IDENTIFIER) {
				stmt->columns.push_back(std::make_unique<Literal>(LiteralValue{ m_currentToken.lexeme }));
				advance();
				if (m_currentToken.type == TokenKind::COMMA) {
					advance();
				}
				else {
					break;
				}
			}
		}
		if (m_currentToken.lexeme != "FROM") {
			throw std::runtime_error("Expected FROM after column list");
		}
		advance();
		if (m_currentToken.type != TokenKind::IDENTIFIER) {
			throw std::runtime_error("Expected table name after FROM");
		}
		stmt->tableName = m_currentToken.lexeme;
		return stmt;
	}

	std::unique_ptr<Statement> parseInsertStatement() {
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

		return stmt;
	}

	std::unique_ptr<Expression> parseLiteralExpression() {
		if (m_currentToken.type == TokenKind::NUMBER) {
			std::string lexeme = m_currentToken.lexeme;
			advance();
			if (lexeme.find('.') != std::string::npos) {
				return std::make_unique<Literal>(LiteralValue{ std::stof(lexeme) });
			}
			else {
				return std::make_unique<Literal>(LiteralValue{ std::stoi(lexeme) });
			}
		}
		if (m_currentToken.type == TokenKind::STRING) {
			std::string lexeme = m_currentToken.lexeme;
			advance();
			return std::make_unique<Literal>(LiteralValue{ lexeme });
		}
		if (m_currentToken.type == TokenKind::KEYWORD) {
			std::string lexeme = m_currentToken.lexeme;
			advance();
			std::string upperLexeme = lexeme;
			std::transform(upperLexeme.begin(), upperLexeme.end(), upperLexeme.begin(), ::toupper);

			if (upperLexeme == "TRUE") {
				return std::make_unique<Literal>(LiteralValue{ true });
			}
			else if (upperLexeme == "FALSE") {
				return std::make_unique<Literal>(LiteralValue{ false });
			}
		}
		throw std::logic_error("Expected literal");
	}

public:
	Parser(Lexer& lexer) : m_lexer(lexer) {
		advance();
	}

	std::unique_ptr<Statement> parseStatement() {
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
				else if (m_currentToken.lexeme == "COLUMN") {
					return parseCreateColumnStatement();
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
				else if (m_currentToken.lexeme == "COLUMN") {
					return parseDropColumnStatement();
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
		auto it = stringToDataTypeMap.find(input);
		if (it != stringToDataTypeMap.end()) {
			return it->second;
		}

		throw std::runtime_error("Unknown DataType: " + input);
	}
};