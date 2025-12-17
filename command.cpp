#include "command.h"
#include "storage.h"
#include "user.h"
#include "book.h"
#include "transaction.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>

inline Storage storage;
ParsedCommand parse_command(const std::string& line){
    ParsedCommand cmd;
    std::string trimmed = trim(line);
    if (trimmed.empty()){
        return cmd;
    }

    std::stringstream ss(trimmed);
    std::string token;
    std::vector<std::string> tokens;

    // 解析第一个token（命令名）
    if (!(ss >> token)) return cmd;
    cmd.name = token;

    // 处理特殊命令：show finance
    if (cmd.name == "show" && ss >> token && token == "finance") {
        cmd.args.push_back("finance");
        if (ss >> token) {
            cmd.args.push_back(token);
        }
        return cmd;
    }

    // 解析剩余tokens
    while (ss >> token) {
        // 检查是否是选项（以-开头）
        if (token[0] == '-') {
            size_t equal_pos = token.find('=');
            if (equal_pos != std::string::npos) {
                std::string key = token.substr(1, equal_pos - 1);
                std::string value = token.substr(equal_pos + 1);

                // 去除引号
                if (value.size() >= 2 && value[0] == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }

                cmd.options[key] = value;
            } else {
                // 如果不是键值对格式，作为普通参数
                cmd.args.push_back(token);
            }
        } else {
            cmd.args.push_back(token);
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
                // 格式：passwd [UserID] [NewPassword] (当前权限为7时)
                if (state.getCurrentPrivilege() != 7) return false;
                new_passwd = cmd.args[1];
            } else if (cmd.args.size() == 3) {
                // 格式：passwd [UserID] [CurrentPassword] [NewPassword]
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

            show_finance(storage, count);
            return true;
        }
            // 处理 show book
        else {
            if (state.getCurrentPrivilege() < 1) return false;

            std::vector<Book> books;

            // 检查是否有参数
            bool has_option = false;
            for (const auto& opt : cmd.options){
                if (!opt.second.empty()){
                    has_option = true;
                    break;
                }
            }

            if (!has_option && cmd.options.empty()){
                // 无参数：显示所有图书
                books = show_books(storage);
            } else {
                // 检查参数内容是否为空
                for (const auto& opt : cmd.options){
                    if (opt.second.empty()) return false;
                }

                if (cmd.options.find("ISBN") != cmd.options.end()){
                    books = show_books(storage, "ISBN", cmd.options.at("ISBN"));
                } else if (cmd.options.find("name") != cmd.options.end()){
                    books = show_books(storage, "name", cmd.options.at("name"));
                } else if (cmd.options.find("author") != cmd.options.end()){
                    books = show_books(storage, "author", cmd.options.at("author"));
                } else if (cmd.options.find("keyword") != cmd.options.end()){
                    books = show_books(storage, "keyword", cmd.options.at("keyword"));
                } else {
                    return false;
                }
            }

            // 输出图书信息
            if (books.empty()){
                std::cout << std::endl;
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
        if (state.selected_isbn.empty()) return false;

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
