#ifndef BOOK_H
#define BOOK_H
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "storage.h"

struct Book{
    std::string isbn;
    std::string name;
    std::string author;
    std::vector<std::string> keywords;
    double price;
    int quantity;
    Book() : price(0.0), quantity(0) {}
    bool valid() const {
        return !isbn.empty();
    }
    bool operator<(const Book& other) const {
        return isbn < other.isbn;
    }
};

std::vector<Book> show_books(Storage& storage, const std::string& condition_type = "", const std::string& condition_value = "");
bool select_book(Storage& storage, SystemState& state, const std::string& isbn);
bool modify_book(Storage& storage, SystemState& state, const std::vector<std::pair<std::string, std::string>>& modifications);
bool import_book(Storage& storage, SystemState& state, int quantity, double total_cost);
double buy_book(Storage& storage, SystemState& state, const std::string& isbn, int quantity);

#endif
