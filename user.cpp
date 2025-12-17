#include "user.h"
#include "command.h"
#include "utils.h"
#include "storage.h"
#include <iostream>

bool register_user(Storage& storage, const std::string& user_id, const std::string& password, const std::string& user_name){
    if (!valid_userid(user_id) || !valid_password(password) || user_name.empty()){
        return false;
    }
    User existing = storage.load_user(user_id);
    if (existing.valid()) return false;

    User new_user;
    new_user.id = user_id;
    new_user.name = user_name;
    new_user.password = password;
    new_user.privilege = 1;
    return storage.save_user(new_user);
}

bool login_user(Storage& storage, SystemState& state, const std::string& user_id, const std::string& password){
    if (!valid_userid(user_id)) return false;

    User user = storage.load_user(user_id);
    if (!user.valid()) return false;

    if (!password.empty()){
        // 提供了密码，需要验证密码
        if (user.password != password){
            return false;
        }
    } else {
        // 没有提供密码，需要当前权限高于用户权限
        int current_privilege = state.getCurrentPrivilege();
        if (current_privilege <= user.privilege) return false;
    }

    state.push_login(user.id, user.privilege);
    state.clear_selected();
    return true;
}

bool logout_user(SystemState& state){
    if (state.login_stack.empty()){
        return false;
    }
    state.pop_login();
    return true;
}

bool change_password(Storage& storage, SystemState& state, const std::string& user_id, const std::string& old_password, const std::string& new_password){
    if (!valid_userid(user_id) || !valid_password(new_password)) return false;

    User user = storage.load_user(user_id);
    if (!user.valid()) return false;

    int current_privilege = state.getCurrentPrivilege();
    if (current_privilege != 7){
        if (old_password.empty()) return false;
        else if (user.password != old_password) return false;
    }

    user.password = new_password;
    return storage.save_user(user);
}

bool add_user(Storage& storage, SystemState& state, const std::string& user_id, const std::string& password, int privilege, const std::string& user_name){
    if (!valid_userid(user_id) || !valid_password(password) || user_name.empty() || (privilege != 1 && privilege != 3)){
        return false;
    }

    int current_priv = state.getCurrentPrivilege();
    if (current_priv <= privilege) return false;

    User existing = storage.load_user(user_id);
    if (existing.valid()) return false;

    User new_user;
    new_user.id = user_id;
    new_user.name = user_name;
    new_user.password = password;
    new_user.privilege = privilege;
    return storage.save_user(new_user);
}

bool delete_user(Storage& storage, SystemState& state, const std::string& user_id){
    if (state.getCurrentPrivilege() != 7) return false;

    User user = storage.load_user(user_id);
    if (!user.valid()) return false;

    for (const auto& entry : state.login_stack){
        if (entry.user_id == user_id) return false;
    }

    return storage.delete_user(user_id);
}