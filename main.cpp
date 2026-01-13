#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"
#include "executor/Executor.hpp"
#include "storage-engine/table_manager.hpp"

volatile sig_atomic_t interrupted = 0;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        interrupted = 1;
        std::cout << "\n^C\ncholopdb> " << std::flush;
    }
}

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

void showHelp() {
    std::cout << "\nAvailable Commands:\n";
    std::cout << "  help     - Show this help message\n";
    std::cout << "  history  - Show command history\n";
    std::cout << "  exit     - Exit the CLI\n";
    std::cout << "  Ctrl+C   - Cancel current input\n";
    std::cout << "\nSQL Commands: CREATE, DROP, USE, SELECT, INSERT, DELETE\n" << std::endl;
    std::cout << "For more information, refer to the CholopDB documentation.\n" << std::endl;
}

void showHistory(const std::vector<std::string>& history) {
    std::cout << "\nCommand History:\n";
    for (size_t i = 0; i < history.size(); i++) {
        std::cout << "  " << i + 1 << ". " << history[i] << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    signal(SIGINT, signalHandler);
    
    std::cout << "==================================" << std::endl;
    std::cout << "     CholopDB SQL Interface       " << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Type 'help' for commands or 'exit' to leave\n" << std::endl;
    
    amk::Executor executor;
    std::vector<std::string> history;
    std::string input;
    
    while (true) {
        std::cout << "cholopdb> " << std::flush;
        
        if (!std::getline(std::cin, input)) {
            break;
        }
        
        if (interrupted) {
            interrupted = 0;
            continue;
        }
        
        size_t start = input.find_first_not_of(" \t\n\r");
        size_t end = input.find_last_not_of(" \t\n\r");
        
        if (start == std::string::npos) {
            continue;
        }
        
        input = input.substr(start, end - start + 1);
        
        if (input == "exit" || input == "quit") {
            std::cout << "Goodbye!" << std::endl;
            break;
        }
        else if (input == "help") {
            showHelp();
            continue;
        }
        else if (input == "history") {
            showHistory(history);
            continue;
        }
        
        if (!input.empty()) {
            history.push_back(input);
            processSQL(executor, input);
        }
        
        std::cout << std::endl;
    }
    
    return 0;
}