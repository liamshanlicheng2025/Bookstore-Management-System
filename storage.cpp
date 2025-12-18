#include "storage.h"
#include "user.h"
#include "book.h"
#include "transaction.h"
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(dir) _mkdir(dir)
#else
#define MKDIR(dir) mkdir(dir, 0755)
#endif

Storage::Storage() :
        user_db("data/users.db"),
        book_db("data/books.db"),
        trans_db("data/transactions.db"),
        finance_db("data/finance.db"),
        data_dir("data") {}

Storage::~Storage() {
    cleanup();
}

bool Storage::initialize(){
    struct stat st;
    if (stat("data", &st) != 0){
        if (MKDIR("data") != 0){
            return false;
        }
    }
    User root = load_user("root");
    if (!root.valid()){
        root.id = "root";
        root.name = "manager";
        root.password = "sjtu";
        root.privilege = 7;
        if (!save_user(root)){
            std::cerr << "Failed to create root user" << std::endl;
            return false;
        }
    }
    return true;
}

void Storage::cleanup(){}

std::string Storage::serialize_user(const User& user){
    std::stringstream ss;
    ss << user.id << "|" << user.name << "|" << user.password << "|" << user.privilege;
    return ss.str();
}

User Storage::deserialize_user(const std::string& data){
    User user;
    std::vector<std::string> parts;
    std::string current;

    for (char c : data) {
        if (c == '|') {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }

    if (parts.size() >= 4){
        user.id = parts[0];
        user.name = parts[1];
        user.password = parts[2];
        try {
            user.privilege = std::stoi(parts[3]);
        } catch(...) {
            user.privilege = 0;
        }
    }
    return user;
}

std::string Storage::serialize_book(const Book& book){
    std::stringstream ss;
    ss << book.isbn << "|" << book.name << "|" << book.author << "|";

    // 序列化keywords，用逗号分隔
    for (size_t i = 0; i < book.keywords.size(); i++){
        if (i > 0) ss << "@";
        ss << book.keywords[i];
    }

    ss << "|" << std::fixed << std::setprecision(2) << book.price
       << "|" << book.quantity;
    return ss.str();
}

Book Storage::deserialize_book(const std::string& data){
    Book book;
    std::vector<std::string> parts;
    std::string current;
    for (char c : data) {
        if (c == '|') {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    // 添加最后一个部分
    if (!current.empty()) {
        parts.push_back(current);
    }
    if (parts.size() >= 6){
        book.isbn = parts[0];
        book.name = parts[1];
        book.author = parts[2];
        // 解析keywords，保持原始顺序
        if (!parts[3].empty()){
            std::vector<std::string> keywords;
            std::string keyword_str = parts[3];
            std::string keyword;
            // 使用逗号分割，但不改变顺序
            for (char c : keyword_str) {
                if (c == '@') {
                    if (!keyword.empty()) {
                        keywords.push_back(keyword);
                        keyword.clear();
                    }
                } else {
                    keyword += c;
                }
            }
            // 添加最后一个keyword
            if (!keyword.empty()) {
                keywords.push_back(keyword);
            }
            book.keywords = keywords;
        }
        try {
            book.price = std::stod(parts[4]);
        } catch(...) {
            book.price = 0.0;
        }
        try {
            book.quantity = std::stoi(parts[5]);
        } catch(...) {
            book.quantity = 0;
        }
    }
    return book;
}

std::string Storage::serialize_trans(const Transaction& trans){
    std::stringstream ss;
    ss << trans.trans_id << "|" << trans.type << "|" << trans.isbn << "|" << trans.quantity
       << "|" << std::fixed << std::setprecision(2) << trans.price << "|" << std::fixed
       << std::setprecision(2) << trans.total << "|" << trans.user_id << "|" << trans.timestamp;
    return ss.str();
}

Transaction Storage::deserialize_trans(const std::string& data){
    Transaction trans;
    std::vector<std::string> parts = split_string(data, '|');
    if (parts.size() >= 8){
        trans.trans_id = parts[0];
        trans.type = parts[1];
        trans.isbn = parts[2];
        trans.quantity = std::stoi(parts[3]);
        trans.price = std::stod(parts[4]);
        trans.total = std::stod(parts[5]);
        trans.user_id = parts[6];
        trans.timestamp = std::stoll(parts[7]);
    }
    return trans;
}

bool Storage::save_user(const User& user){
    std::string key = "user:" + user.id;
    std::string value = serialize_user(user);
    return user_db.insert_or_update(key, value);
}

User Storage::load_user(const std::string& user_id){
    std::string key = "user:" + user_id;
    std::string data = user_db.find(key);
    if (data.empty()) {
        return User();
    }
    return deserialize_user(data);
}

bool Storage::delete_user(const std::string& user_id){
    std::string key = "user:" + user_id;
    return user_db.remove(key);
}

std::vector<User> Storage::get_all_users(){
    std::vector<User> users;
    auto all = user_db.find_prefix("user:");
    for (const auto& entry : all){
        User user = deserialize_user(entry.second);
        if (!user.id.empty()) users.push_back(user);
    }
    return users;
}

bool Storage::save_book(const Book& book){
    std::string key = "book:" + book.isbn;
    std::string value = serialize_book(book);
    return book_db.insert(key, value) || book_db.update(key, value);
}

Book Storage::load_book(const std::string& isbn){
    std::string key = "book:" + isbn;
    std::string data = book_db.find(key);
    if (data.empty()) return Book();
    return deserialize_book(data);
}

bool Storage::delete_book(const std::string& isbn){
    std::string key = "book:" + isbn;
    return book_db.remove(key);
}

std::vector<Book> Storage::get_all_books(){
    std::vector<Book> books;
    auto all = book_db.find_prefix("book:");
    for (const auto& entry : all){
        Book book = deserialize_book(entry.second);
        if (!book.isbn.empty()) {
            // 检查是否已经存在相同ISBN的书
            bool exists = false;
            for (const auto& existing_book : books){
                if (existing_book.isbn == book.isbn){
                    exists = true;
                    break;
                }
            }
            if (!exists){
                books.push_back(book);
            }
        }
    }
    // 按ISBN升序排序
    std::sort(books.begin(), books.end(), [](const Book& a, const Book& b){
        return a.isbn < b.isbn;
    });
    return books;
}

std::vector<Book> Storage::get_books_by_keyword(const std::string& keyword){
    std::vector<Book> all = get_all_books();
    std::vector<Book> result;
    for (const auto& book : all){
        for (const auto& kw : book.keywords){
            if (kw == keyword){
                result.push_back(book);
                break;
            }
        }
    }
    return result;
}

std::vector<Book> Storage::get_books_by_author(const std::string& author){
    std::vector<Book> all = get_all_books();
    std::vector<Book> result;
    for (const auto& book : all){
        if (book.author == author){
            result.push_back(book);
        }
    }
    return result;
}

bool Storage::save_transaction(const Transaction& trans){
    std::string key = "trans:" + trans.trans_id;
    std::string value = serialize_trans(trans);
    bool success = trans_db.insert(key, value);
    if (success){
        if (trans.type == "buy"){
            update_finance(trans.total, 0.0);
        } else {
            update_finance(0.0, trans.total);
        }
    }
    return success;
}

std::vector<Transaction> Storage::get_all_transactions() {
    std::vector<Transaction> transactions;
    auto all = trans_db.find_prefix("trans:");
    for (const auto& entry : all){
        Transaction trans = deserialize_trans(entry.second);
        if (!trans.trans_id.empty()) transactions.push_back(trans);
    }
    std::sort(transactions.begin(), transactions.end(), [](const Transaction& a, const Transaction& b){
        return a.timestamp < b.timestamp;
    });
    return transactions;
}

std::vector<Transaction> Storage::get_recent_transactions(int count){
    std::vector<Transaction> all = get_all_transactions();
    if (count <= 0 || count >= (int)all.size()){
        return all;
    }
    std::vector<Transaction> result;
    for (int i = (int)all.size() - count; i < (int)all.size(); i++){
        result.push_back(all[i]);
    }
    return result;
}

void Storage::update_finance(double income, double expense){
    std::string current = finance_db.find("summary");
    double total_income = 0.0, total_expense = 0.0;
    if (!current.empty()){
        std::vector<std::string> parts = split_string(current, '|');
        if (parts.size() >= 2){
            total_income = std::stod(parts[0]);
            total_expense = std::stod(parts[1]);
        }
    }
    total_income += income;
    total_expense += expense;
    std::string new_value = to_string(total_income) + "|" + to_string(total_expense);
    finance_db.insert("summary", new_value) || finance_db.update("summary", new_value);
}

std::pair<double, double> Storage::get_finance_summary(int count){
    if (count == 0){
        return make_pair(0.0, 0.0);
    }
    std::vector<Transaction> transactions;
    if (count < 0){
        transactions = get_all_transactions();
    } else {
        transactions = get_recent_transactions(count);
    }
    double total_income = 0.0, total_expense = 0.0;
    for (const auto& trans : transactions){
        if (trans.type == "buy"){
            total_income += trans.total;
        } else if (trans.type == "import"){
            total_expense += trans.total;
        }
    }
    return make_pair(total_income, total_expense);
}

bool Storage::save_state(const SystemState& state){
    std::stringstream ss;
    ss << state.login_stack.size() << std::endl;
    for (const auto& entry : state.login_stack){
        ss << entry.user_id << " " << entry.privilege << std::endl;
    }
    ss << state.selected_isbn << std::endl;
    std::string key = "system_state";
    std::string value = ss.str();
    return user_db.insert(key, value) || user_db.update(key, value);
}

bool Storage::load_state(SystemState& state){
    std::string data = user_db.find("system_state");
    if (data.empty()) return false;
    std::stringstream ss(data);
    int stack_size;
    ss >> stack_size;
    state.login_stack.clear();
    for (int i = 0; i < stack_size; i++){
        std::string user_id;
        int privilege;
        if (ss >> user_id >> privilege){
            LoginEntry entry = {user_id, privilege};
            state.login_stack.push_back(entry);
        }
    }
    ss >> state.selected_isbn;
    return true;
}