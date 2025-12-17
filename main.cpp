#include <iostream>
#include <string>
#include "command.h"
#include "storage.h"
#include "utils.h"

int main(){
    Storage storage;

    if (!storage.initialize()){
        std::cerr << "Fail to initialize!" << std::endl;
        return 1;
    }

    SystemState state;
    storage.load_state(state);

    std::string line;
    while (!state.should_exit){
        if (!getline(std::cin, line)){
            break;
        }
        ParsedCommand cmd = parse_command(line);
        bool success = execute(cmd, state);
        if (!success){
            std::cout << "Invalid" << std::endl;
        }
        if (state.should_exit){
            break;
        }
    }

    storage.save_state(state);
    return 0;
}
// Created by Lenovo on 2025/12/13.
//
