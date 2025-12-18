#include "book.h"
#include "storage.h"
#include "command.h"
#include "transaction.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

std::vector<Book> show_books(Storage& storage, const std::string& condition_type, const std::string& condition_value){
    std::vector<Book> all = storage.get_all_books();
    std::vector<Book> result;
    if (condition_type.empty()) {
        // 没有筛选条件，返回所有图书
        std::sort(all.begin(), all.end(), [](const Book& a, const Book& b){
            return a.isbn < b.isbn;
        });
        return all;
    }
    if (condition_value.empty()) {
        return result;
    }
    for (const auto& book : all){
        bool match = false;
        if (condition_type == "ISBN") {
            match = (book.isbn == condition_value);
        }
        else if (condition_type == "name") {
            match = (book.name == condition_value);
        }
        else if (condition_type == "author") {
            match = (book.author == condition_value);
        }
        else if (condition_type == "keyword") {
            for (const auto& kw : book.keywords){
                if (kw == condition_value){
                    match = true;
                    break;
                }
            }
        }
        if (match) {
            result.push_back(book);
        }
    }
    // 按ISBN升序排序
    std::sort(result.begin(), result.end(), [](const Book& a, const Book& b){
        return a.isbn < b.isbn;
    });

    return result;
}
bool select_book(Storage& storage, SystemState& state, const std::string& isbn){
    if (state.getCurrentPrivilege() < 3) return false;
    if (!valid_isbn(isbn)) return false;
    Book book = storage.load_book(isbn);
    if (!book.valid()){
        Book new_book;
        new_book.isbn = isbn;
        new_book.name = "";
        new_book.author = "";
        new_book.keywords.clear();
        new_book.price = 0.0;
        new_book.quantity = 0;
        if (!storage.save_book(new_book)) {
            return false;
        }
    }
    state.selected_isbn = isbn;
    return true;
}
bool modify_book(Storage& storage, SystemState& state, const std::vector<std::pair<std::string, std::string>>& modifications){
    if (state.selected_isbn.empty()) return false;
    Book book = storage.load_book(state.selected_isbn);
    if (!book.valid()) return false;
    // 检查重复参数
    std::vector<std::string> seen_params;
    for (const auto& mod : modifications){
        if (std::find(seen_params.begin(), seen_params.end(), mod.first) != seen_params.end()){
            return false;
        }
        seen_params.push_back(mod.first);
        if (mod.second.empty()) return false;
    }
    std::string old_isbn = book.isbn;
    bool isbn_changed = false;
    std::string new_isbn;
    // 首先检查是否有ISBN修改
    for (const auto& mod : modifications){
        if (mod.first == "ISBN"){
            if (mod.second == old_isbn) return false;
            if (!valid_isbn(mod.second)) return false;

            // 检查新ISBN是否已存在
            Book existing_book = storage.load_book(mod.second);
            if (existing_book.valid()) return false;

            new_isbn = mod.second;
            isbn_changed = true;
            break;
        }
    }
    // 应用所有修改
    for (const auto& mod : modifications){
        std::string type = mod.first;
        std::string value = mod.second;
        if (type == "ISBN"){
            book.isbn = value;
        }
        else if (type == "name"){
            if (!valid_bookname(value)) return false;
            book.name = value;
        }
        else if (type == "author"){
            if (!valid_author(value)) return false;
            book.author = value;
        }
        else if (type == "keyword"){
            std::vector<std::string> keywords;
            std::stringstream ss(value);
            std::string item;
            // 用|分割关键词，保持原始顺序
            while (std::getline(ss, item, '|')) {
                if (item.empty()) return false;
                // 检查重复关键词（按顺序检查，不排序）
                if (std::find(keywords.begin(), keywords.end(), item) != keywords.end()) {
                    return false; // 发现重复关键词
                }
                keywords.push_back(item);
            }
            book.keywords = keywords;
        }
        else if (type == "price"){
            if (!valid_price(value)) return false;
            book.price = std::stod(value);
        }
    }
    if (isbn_changed){
        if (!storage.save_book(book)) return false;
        storage.delete_book(old_isbn);
        state.selected_isbn = new_isbn;
    }
    else {
        // ISBN未改变，直接保存
        if (!storage.save_book(book)) return false;
    }
    return true;
}
bool import_book(Storage& storage, SystemState& state, int quantity, double total_cost){
    if (state.getCurrentPrivilege() < 3) return false;
    if (state.selected_isbn.empty()) return false;
    if (quantity <= 0 || total_cost <= 0) return false;
    Book book = storage.load_book(state.selected_isbn);
    if (!book.valid()) return false;
    book.quantity += quantity;
    if (!storage.save_book(book)) return false;
    Transaction trans;
    trans.trans_id = generate_trans_id();
    trans.type = "import";
    trans.isbn = state.selected_isbn;
    trans.quantity = quantity;
    trans.price = total_cost / quantity;
    trans.total = total_cost;
    trans.user_id = state.getCurrentUserId();
    trans.timestamp = time(nullptr);
    if (!storage.save_transaction(trans)){
        book.quantity -= quantity;
        storage.save_book(book);
        return false;
    }
    return true;
}
double buy_book(Storage& storage, SystemState& state, const std::string& isbn, int quantity){
    if (state.getCurrentPrivilege() < 1) return -1.0;
    if (!valid_isbn(isbn) || quantity <= 0) return -1.0;
    Book book = storage.load_book(isbn);
    if (!book.valid()) return -1.0;
    if (book.quantity < quantity) return -1.0;
    double total = book.price * quantity;
    book.quantity -= quantity;
    if (!storage.save_book(book)) return -1.0;
    Transaction trans;
    trans.trans_id = generate_trans_id();
    trans.type = "buy";
    trans.isbn = isbn;
    trans.quantity = quantity;
    trans.price = book.price;
    trans.total = total;
    trans.user_id = state.getCurrentUserId();
    trans.timestamp = time(nullptr);
    if (!storage.save_transaction(trans)){
        book.quantity += quantity;
        storage.save_book(book);
        return -1.0;
    }
    return total;
}
// Created by Lenovo on 2025/12/13.
//
