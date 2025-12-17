#ifndef USER_H
#define USER_H
#include <string>
#include <ctime>
#include <vector>
#include "storage.h"

struct User{
    std::string id;
    std::string name;
    std::string password;
    int privilege;
    User() : privilege(0) {}
    bool valid() const {
        return !id.empty() && !name.empty();
    }
};
bool register_user(Storage& storage, const std::string& user_id, const std::string& password, const std::string& user_name);
bool login_user(Storage& storage, SystemState& state, const std::string& user_id, const std::string& password = "");
bool logout_user(SystemState& state);
bool change_password(Storage& storage, SystemState& state, const std::string& user_id, const std::string& old_password, const std::string& new_password);
bool add_user(Storage& storage, SystemState& state, const std::string& user_id, const std::string& password, int privilege, const std::string& user_name);
bool delete_user(Storage& storage, SystemState& state, const std::string& user_id);
#endif
