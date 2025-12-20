#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <string>
#include <cstdint>
#include "storage.h"
#include "user.h"

struct Transaction{
    std::string trans_id;
    std::string type;
    std::string isbn;
    int quantity;
    double price;
    double total;
    std::string user_id;
    int64_t timestamp;
    Transaction() : quantity(0), price(0.0), total(0.0), timestamp(0) {}
    bool valid() const {
        return !trans_id.empty() && !isbn.empty();
    }
};

void show_finance(Storage& storage, int count = -1);
void report_finance(Storage& storage);
void report_employee(Storage& storage, SystemState& state);
void report_log(Storage& storage);

#endif
