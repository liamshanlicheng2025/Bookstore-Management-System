#include "utils.h"
#include <cstring>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

std::vector<std::string> split_string(const std::string& str, char delimiter){
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (getline(ss, item, delimiter)){
        if (!item.empty()){
            result.push_back(trim(item));
        }
    }
    return result;
}

std::string trim(const std::string& str){
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

bool start_with(const std::string& str, const std::string& prefix){
    if (prefix.size() > str.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool valid_userid(const std::string& str){
    if (str.empty() || str.length() > 30) return false;
    for (char c : str){
        if (!(std::isalnum(c) || c == '_')) return false;
    }
    return true;
}

bool valid_password(const std::string& str){
    return valid_userid(str);
}

bool valid_isbn(const std::string& str){
    if (str.empty() || str.length() > 20) return false;
    for (char c : str){
        if (!std::isprint(c)) return false;
    }
    return true;
}

bool valid_bookname(const std::string& str){
    if (str.empty() || str.length() > 60) return false;
    for (char c : str){
        if (!std::isprint(c) || c == '"') return false;
    }
    return true;
}

bool valid_author(const std::string& str){
    return valid_bookname(str);
}

bool valid_keyword(const std::string& str){
    if (str.empty() || str.length() > 60) return false;
    for (char c : str){
        if (!std::isprint(c) || c == '"') return false;
    }
    return true;
}

bool valid_price(const std::string& str){
    try{
        double price = std::stod(str);
        if (price < 0) return false;
        return true;
    } catch(...){
        return false;
    }
}

bool valid_quantity(const std::string& str){
    try{
        int quantity = std::stoi(str);
        if (quantity < 0) return false;
        return true;
    } catch(...){
        return false;
    }
}

std::string format_double(double value){
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

std::string format_time(time_t timestamp) {
    tm* timeinfo = localtime(&timestamp);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

std::string generate_id() {
    static int counter = 0;
    return "ID" + std::to_string(time(nullptr)) + std::to_string(counter++);
}

std::string generate_trans_id() {
    static int counter = 0;
    return "TR" + std::to_string(time(nullptr)) + std::to_string(counter++);
}

bool check_privilege(int required, int current){
    return required <= current;
}

