#pragma once
#include <string>
#include <algorithm>
#include <vector>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include "AST.hpp"

enum class TokenKind {
	KEYWORD,
    IDENTIFIER,
    //Literals
	NUMBER, STRING,
	DATA_TYPE,
	//Punctuation and Operators
	LEFT_PAREN, RIGHT_PAREN, COMMA, SEMI_COLON, ASTERISK,
    EQUAL, LESS_THAN, GREATER_THAN,
    END_OF_FILE,
};

std::vector <std::string> keywordList = {
    "SELECT", "FROM", "INSERT", "INTO", "VALUES", "WHERE",
    "SET", "DELETE", "DROP",
	"CREATE", "COLUMN", "DATABASE", "TABLE", "USE",
    "TRUE", "FALSE"
};

static const std::unordered_map<std::string, DataType> stringToDataTypeMap = {
    {"SMALLINT",  DataType::SMALLINT},
    {"BIGINT",    DataType::BIGINT},
    {"FLOAT",     DataType::FLOAT},
    {"SMALLTEXT", DataType::SMALLTEXT},
    {"BIGTEXT",   DataType::BIGTEXT},
    {"ID",        DataType::ID}
};

struct Token {
    TokenKind type;
    std::string lexeme;
};

class Lexer {
private:
    std::string m_input;
    size_t m_index = 0;
    bool m_reachedEOF = false;
    
    char next() {
        if (m_index >= m_input.size()) {
            m_reachedEOF = true;
            return '\0';
        }
        else {
            return m_input[m_index++];
        }

    }
public:
    Lexer(const std::string& input) : m_input(input), m_index(0) {}

    Token getNextToken() {
        
        char c = next();
		while ( isspace(static_cast<unsigned char>(c)) ) {
            c = next();
        }
        
        if (m_reachedEOF) {
            return Token{ TokenKind::END_OF_FILE, ""};
        }

        size_t token_start = m_index - 1;
        if (isalpha(static_cast<unsigned char>(c))) {
            std::string lexeme;

            do {
                lexeme += c;
                c = next();
            } 
            while (isalnum(static_cast<unsigned char>(c)));
            
            m_index--;
            auto it = std::find(keywordList.begin(), keywordList.end(), lexeme);
            if (it != keywordList.end()) {
                return Token{ TokenKind::KEYWORD, lexeme};
            }

            if (stringToDataTypeMap.find(lexeme) != stringToDataTypeMap.end()) {
                return Token{ TokenKind::DATA_TYPE, lexeme };
            }

            return Token{ TokenKind::IDENTIFIER, lexeme};
        }

        else if ( isdigit(static_cast<unsigned char>(c)) ) {
            std::string lexeme;
            bool has_dot = false;

            do {
                if (c == '.') {
                    if (has_dot) {
                        throw std::runtime_error("Invalid number format: multiple decimal points.");
                    }
                    has_dot = true;
                }
                lexeme += c;
                c = next();
            } 
            while (isdigit(static_cast<unsigned char>(c)) || c == '.');
            
            m_index--;
            return Token{ TokenKind::NUMBER, lexeme};
        }

        else {
            switch (c) {
            case '*':
                return Token{ TokenKind::ASTERISK, "*"};
            case '(':
                return Token{ TokenKind::LEFT_PAREN, "("};
            case ')':
                return Token{ TokenKind::RIGHT_PAREN, ")"};
            case ',':
                return Token{ TokenKind::COMMA, ","};
            case ';':
                return Token{ TokenKind::SEMI_COLON, ";"};
            case '=':
                return Token{ TokenKind::EQUAL, "="};
            case '<':
                return Token{ TokenKind::LESS_THAN, "<"};
            case '>':
                return Token{ TokenKind::GREATER_THAN, ">"};
            case '"': {
                std::string lexeme;
                char ch = next();
                while (ch != '"' && !m_reachedEOF) {
                    lexeme += ch;
                    ch = next();
                }
                if (m_reachedEOF && ch != '"') {  
                    throw std::runtime_error("Unterminated string literal");
                }
                return Token{ TokenKind::STRING, lexeme };
            }
            default:
                throw std::runtime_error("Unknown operator");

            return Token{ TokenKind::END_OF_FILE, "" };
            }
        }
    }
};