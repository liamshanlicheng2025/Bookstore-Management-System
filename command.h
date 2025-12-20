#ifndef COMMAND_H
#define COMMAND_H
#include <string>
#include <vector>
#include <map>
struct LoginEntry{
    std::string user_id;
    int privilege;
    std::string selected_isbn;
    LoginEntry(const std::string& id = "", int priv = 0)
        : user_id(id), privilege(priv), selected_isbn("") {}
};

struct SystemState{
    std::vector<LoginEntry> login_stack; //登录栈
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
        if (!login_stack.empty()){
            login_stack.back().selected_isbn.clear();
        }
    }
    std::string getSelectedIsbn() const {
        if (!login_stack.empty()){
            return login_stack.back().selected_isbn;
        }
        return "";
    }
    void setSelectedIsbn(const std::string& isbn){
        if (!login_stack.empty()){
            login_stack.back().selected_isbn = isbn;
        }
    }
    void updateSelectedIsbnAll(const std::string& old_isbn, const std::string& new_isbn){
        if (old_isbn == new_isbn) return;
        for (auto& entry : login_stack){
            if (entry.selected_isbn == old_isbn){
                entry.selected_isbn = new_isbn;
            }
        }
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
