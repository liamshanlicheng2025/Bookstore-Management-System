#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cctype>

std::vector<std::string> split_string(const std::string& str, char delimiter);
std::string trim(const std::string& str);
bool start_with(const std::string& str, const std::string& prefix);

bool valid_userid(const std::string& str);
bool valid_password(const std::string& str);
bool valid_isbn(const std::string& str);
bool valid_bookname(const std::string& str);
bool valid_author(const std::string& str);
bool valid_keyword(const std::string& str);
bool valid_price(const std::string& str);
bool valid_quantity(const std::string& str);

std::string format_double(double value);
std::string format_time(time_t timestamp);

std::string generate_id();
std::string generate_trans_id();

bool check_privilege(int required, int current);

#endif

