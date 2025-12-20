#include "command.h"
#include "storage.h"
#include "user.h"
#include "book.h"
#include "transaction.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>

extern Storage storage;
ParsedCommand parse_command(const std::string& line){
    ParsedCommand cmd;
    std::string trimmed = trim(line);
    if (trimmed.empty()){
        return cmd;
    }
    std::stringstream ss(trimmed);
    std::string token;
    std::vector<std::string> tokens;
    // 分割所有token
    while (ss >> token){
        tokens.push_back(token);
    }
    if (tokens.empty()){
        return cmd;
    }
    cmd.name = tokens[0];
    if (cmd.name == "show" && tokens.size() > 1 && tokens[1] == "finance"){
        cmd.args.push_back("finance");
        if (tokens.size() > 2){
            cmd.args.push_back(tokens[2]);
        }
        return cmd;
    }
    for (size_t i = 1; i < tokens.size(); i++){
        std::string arg = tokens[i];
        // 检查是否是选项（以-开头且包含=）
        if (arg[0] == '-' && arg.find('=') != std::string::npos){
            size_t pos = arg.find('=');
            std::string key = arg.substr(1, pos - 1);
            std::string value = arg.substr(pos + 1);
            // 去除引号
            if (!value.empty() && value.front() == '"' && value.back() == '"'){
                value = value.substr(1, value.size() - 2);
            }
            cmd.options[key] = value;
        } else {
            cmd.args.push_back(arg);
        }
    }

    return cmd;
}

bool execute(const ParsedCommand& cmd, SystemState& state){
    if (cmd.name.empty()){
        return true;
    }
    if (cmd.name == "quit" || cmd.name == "exit"){
        state.should_exit = true;
        return true;
    }
    else if (cmd.name == "su"){
        if (cmd.args.size() == 1 || cmd.args.size() == 2){
            std::string user_id = cmd.args[0];
            std::string password = cmd.args.size() == 2 ? cmd.args[1] : "";
            return login_user(storage, state, user_id, password);
        }
    }
    else if (cmd.name == "logout"){
        return logout_user(state);
    }
    else if (cmd.name == "register"){
        if (cmd.args.size() == 3){
            return register_user(storage, cmd.args[0], cmd.args[1], cmd.args[2]);
        }
    }
    else if (cmd.name == "passwd"){
        if (cmd.args.size() == 2 || cmd.args.size() == 3){
            std::string user_id = cmd.args[0];
            std::string old_passwd = "";
            std::string new_passwd = "";
            if (cmd.args.size() == 2) {
                if (state.getCurrentPrivilege() != 7) return false;
                new_passwd = cmd.args[1];
            } else if (cmd.args.size() == 3) {
                old_passwd = cmd.args[1];
                new_passwd = cmd.args[2];
            }
            return change_password(storage, state, user_id, old_passwd, new_passwd);
        }
    }
    else if (cmd.name == "useradd"){
        if (cmd.args.size() == 4){
            int privilege;
            try {
                privilege = std::stoi(cmd.args[2]);
            } catch(...) {
                return false;
            }
            if (privilege != 1 && privilege != 3 && privilege != 7) return false;
            return add_user(storage, state, cmd.args[0], cmd.args[1], privilege, cmd.args[3]);
        }
    }
    else if (cmd.name == "delete"){
        if (cmd.args.size() == 1){
            return delete_user(storage, state, cmd.args[0]);
        }
    }
    else if (cmd.name == "show"){
        // 处理 show finance
        if (!cmd.args.empty() && cmd.args[0] == "finance"){
            if (state.getCurrentPrivilege() < 7) return false;
            int count = -1;
            if (cmd.args.size() > 1){
                try {
                    count = std::stoi(cmd.args[1]);
                } catch(...) {
                    return false;
                }
            }
            if (count > 0){
                int total_trans = (int)storage.get_all_transactions().size();
                if (count > total_trans) return false;
            }
            show_finance(storage, count);
            return true;
        }
        else {
            //show book
            if (state.getCurrentPrivilege() < 1) return false;
            std::vector<Book> books;
            // 如果没有选项，显示所有图书
            if (cmd.options.empty() && cmd.args.empty()){
                books = show_books(storage);
            }
            else if (!cmd.options.empty()){
                // 确保只有一个筛选条件
                if (cmd.options.size() > 1) return false;
                auto it = cmd.options.begin();
                std::string condition_type = it->first;
                std::string condition_value = it->second;
                if (condition_value.empty()) return false;
                if (condition_type == "ISBN"){
                    books = show_books(storage, "ISBN", condition_value);
                } else if (condition_type == "name"){
                    books = show_books(storage, "name", condition_value);
                } else if (condition_type == "author"){
                    books = show_books(storage, "author", condition_value);
                } else if (condition_type == "keyword"){
                    books = show_books(storage, "keyword", condition_value);
                } else {
                    return false;
                }
            } else {
                return false;
            }
            if (books.empty()){
                std::cout << std::endl; // 无符合条件的书则输出空行
            } else {
                for (const auto& book : books){
                    std::cout << book.isbn << "\t" << book.name << "\t" << book.author << "\t";
                    // 输出关键词，用|分隔
                    for (size_t i = 0; i < book.keywords.size(); i++){
                        if (i > 0) std::cout << "|";
                        std::cout << book.keywords[i];
                    }
                    std::cout << "\t" << format_double(book.price)
                              << "\t" << book.quantity << std::endl;
                }
            }
            return true;
        }
    }
    else if (cmd.name == "buy"){
        if (cmd.args.size() == 2){
            if (state.getCurrentPrivilege() < 1) return false;
            std::string isbn = cmd.args[0];
            int quantity;
            try {
                quantity = std::stoi(cmd.args[1]);
            } catch(...) {
                return false;
            }
            double total = buy_book(storage, state, isbn, quantity);
            if (total >= 0){
                std::cout << format_double(total) << std::endl;
                return true;
            }
        }
    }
    else if (cmd.name == "select"){
        if (cmd.args.size() == 1){
            if (state.getCurrentPrivilege() < 3) return false;
            return select_book(storage, state, cmd.args[0]);
        }
    }
    else if (cmd.name == "modify"){
        if (state.getCurrentPrivilege() < 3) return false;
        if (state.getSelectedIsbn().empty()) return false;
        if (cmd.options.empty()) return false;
        std::vector<std::pair<std::string, std::string>> modifications;
        for (const auto& opt : cmd.options){
            modifications.push_back({opt.first, opt.second});
        }
        return modify_book(storage, state, modifications);
    }
    else if (cmd.name == "import"){
        if (cmd.args.size() == 2){
            if (state.getCurrentPrivilege() < 3) return false;
            int quantity;
            double total_cost;
            try {
                quantity = std::stoi(cmd.args[0]);
                total_cost = std::stod(cmd.args[1]);
            } catch(...) {
                return false;
            }
            return import_book(storage, state, quantity, total_cost);
        }
    }
    else if (cmd.name == "report"){
        if (cmd.args.size() == 1){
            if (state.getCurrentPrivilege() < 7) return false;

            if (cmd.args[0] == "finance"){
                report_finance(storage);
                return true;
            } else if (cmd.args[0] == "employee"){
                report_employee(storage, state);
                return true;
            }
        }
    }
    else if (cmd.name == "log"){
        if (state.getCurrentPrivilege() < 7) return false;
        report_log(storage);
        return true;
    }
    return false;
}
