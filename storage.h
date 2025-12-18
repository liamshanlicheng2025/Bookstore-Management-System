#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include <map>
#include "BlockListDB.hpp"
#include "command.h"
#include "utils.h"

struct User;
struct Book;
struct Transaction;
struct SystemState;

class Storage {
private:
    BlockListDB user_db;
    BlockListDB book_db;
    BlockListDB trans_db;
    BlockListDB finance_db;
    std::string data_dir;

    // 序列化与反序列化
    std::string serialize_user(const User& user);
    User deserialize_user(const std::string& data);
    std::string serialize_book(const Book& book);
    Book deserialize_book(const std::string& data);
    std::string serialize_trans(const Transaction& trans);
    Transaction deserialize_trans(const std::string& data);

public:
    Storage();
    ~Storage();
    bool initialize();
    void cleanup();

    bool save_user(const User& user);
    User load_user(const std::string& user_id);
    bool delete_user(const std::string& user_id);
    std::vector<User> get_all_users();

    bool save_book(const Book& book);
    Book load_book(const std::string& isbn);
    bool delete_book(const std::string& isbn);
    std::vector<Book> get_all_books();
    std::vector<Book> get_books_by_keyword(const std::string& keyword);
    std::vector<Book> get_books_by_author(const std::string& author);

    bool save_transaction(const Transaction& trans);
    std::vector<Transaction> get_all_transactions();
    std::vector<Transaction> get_recent_transactions(int count);

    void update_finance(double income, double expense);
    std::pair<double, double> get_finance_summary(int count = -1);

    bool save_state(const SystemState& state);
    bool load_state(SystemState& state);
};

#endif // STORAGE_H
