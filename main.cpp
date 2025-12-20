#include <iostream>
#include <string>
#include <cstdlib>
#include "command.h"
#include "storage.h"
#include "utils.h"
Storage storage;
int main(){

    if (!storage.initialize()){
        std::cerr << "Fail to initialize!" << std::endl;
        return 1;
    }

    SystemState state;

    std::string line;
    const char* trace_env = std::getenv("BOOKSTORE_TRACE");
    bool trace = (trace_env != nullptr && *trace_env != '\0');
    size_t line_no = 0;
    while (!state.should_exit){
        if (!getline(std::cin, line)){
            break;
        }
        line_no++;
        ParsedCommand cmd = parse_command(line);
        bool success = execute(cmd, state);
        if (trace){
            std::cerr << "[TRACE] #" << line_no
                      << " cmd=\"" << trim(line) << "\""
                      << " result=" << (success ? "OK" : "FAIL")
                      << " priv=" << state.getCurrentPrivilege()
                      << " stack=" << state.login_stack.size()
                      << " selected=\"" << state.getSelectedIsbn() << "\""
                      << std::endl;
        }
        if (!success){
            std::cout << "Invalid" << std::endl;
        }
        if (state.should_exit){
            break;
        }
    }

    return 0;
}
// Created by Lenovo on 2025/12/13.
//
