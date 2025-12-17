#ifndef COMMAND_H
#define COMMAND_H
#include <string>
#include <vector>
#include <map>
struct LoginEntry{
    std::string user_id;
    int privilege;
    LoginEntry(const std::string& id = "", int priv = 0) : user_id(id), privilege(priv) {}
};

struct SystemState{
    std::vector<LoginEntry> login_stack; //登录栈
    std::string selected_isbn;
    bool should_exit;
    SystemState() : should_exit(false) {}
    int getCurrentPrivilege() const {
        if (!login_stack.empty()){
            return login_stack.back().privilege;
        }
        return 0;
    }
    std::string getCurrentUserId() const {
        if (!login_stack.empty()){
            return login_stack.back().user_id;
        }
        return "";
    }
    void push_login(const std::string& user_id, int privilege){
        LoginEntry entry(user_id, privilege);
        login_stack.push_back(entry);
    }
    bool pop_login(){
        if (!login_stack.empty()){
            login_stack.pop_back();
            return true;
        }
        return false;
    }
    void clear_selected(){
        selected_isbn.clear();
    }
};

struct ParsedCommand{
    std::string name;
    std::vector<std::string> args;
    std::map<std::string, std::string> options; //用于存储查询时的组合
    ParsedCommand() {};
    void clear(){
        name.clear();
        args.clear();
        options.clear();
    }
};

ParsedCommand parse_command(const std::string& line);
bool execute(const ParsedCommand& cmd, SystemState& state);

#endif
