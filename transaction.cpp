#include "transaction.h"
#include "storage.h"
#include "command.h"
#include "utils.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>

void show_finance(Storage& storage, int count) {
    if (count == 0) {
        std::cout << std::endl;
        return;
    }

    std::pair<double, double> finance = storage.get_finance_summary(count);

    const char* trace_env = std::getenv("BOOKSTORE_TRACE");
    if (trace_env != nullptr && *trace_env != '\0') {
        size_t trans_count = storage.get_all_transactions().size();
        std::cerr << "[TRACE_FINANCE] count=" << count
                  << " total_trans=" << trans_count
                  << " income=" << format_double(finance.first)
                  << " expense=" << format_double(finance.second)
                  << std::endl;
    }

    // 输出格式：+ [收入] - [支出]
    std::cout << "+ " << format_double(finance.first)
              << " - " << format_double(finance.second) << std::endl;
}

void report_finance(Storage& storage) {
    std::vector<Transaction> transactions = storage.get_all_transactions();
    std::cout << "=============================================" << std::endl;
    std::cout << "                 财务报表" << std::endl;
    std::cout << "=============================================" << std::endl;
    double total_income = 0.0, total_expense = 0.0;
    for (const auto& trans : transactions) {
        std::cout << "交易ID: " << trans.trans_id << std::endl;
        std::cout << "类型: " << (trans.type == "buy" ? "销售" : "进货") << std::endl;
        std::cout << "ISBN: " << trans.isbn << std::endl;
        std::cout << "数量: " << trans.quantity << std::endl;
        std::cout << "单价: " << format_double(trans.price) << std::endl;
        std::cout << "总额: " << format_double(trans.total) << std::endl;
        std::cout << "用户: " << trans.user_id << std::endl;
        std::cout << "时间: " << format_time(trans.timestamp) << std::endl;
        std::cout << "---------------------------------------------" << std::endl;
        if (trans.type == "buy") {
            total_income += trans.total;
        } else {
            total_expense += trans.total;
        }
    }
    std::cout << "总收入: " << format_double(total_income) << std::endl;
    std::cout << "总支出: " << format_double(total_expense) << std::endl;
    std::cout << "净利润: " << format_double(total_income - total_expense) << std::endl;
    std::cout << "=============================================" << std::endl;
}
void report_employee(Storage& storage, SystemState& state) {
    std::vector<User> users = storage.get_all_users();
    std::vector<Transaction> transactions = storage.get_all_transactions();
    std::cout << "=============================================" << std::endl;
    std::cout << "               员工工作报告" << endl;
    std::cout << "=============================================" << std::endl;
    std::map<std::string, std::vector<Transaction>> user_transactions;
    for (const auto& trans : transactions) {
        user_transactions[trans.user_id].push_back(trans);
    }
    for (const auto& user : users) {
        if (user.privilege >= 3) { // 只显示员工和店长
            std::cout << "员工: " << user.name << " (" << user.id << ")" << std::endl;
            std::cout << "权限: " << user.privilege << std::endl;
            if (user_transactions.find(user.id) != user_transactions.end()) {
                std::cout << "交易记录:" << std::endl;
                for (const auto& trans : user_transactions[user.id]) {
                    std::cout << "  - " << format_time(trans.timestamp)
                         << " " << (trans.type == "buy" ? "销售" : "进货")
                         << " " << trans.isbn
                         << " 数量:" << trans.quantity
                         << " 总额:" << format_double(trans.total) << std::endl;
                }
            } else {
                std::cout << "暂无交易记录" << std::endl;
            }
            std::cout << "---------------------------------------------" << std::endl;
        }
    }
    std::cout << "=============================================" << std::endl;
}
void report_log(Storage& storage) {
    std::vector<Transaction> transactions = storage.get_all_transactions();
    std::cout << "=============================================" << std::endl;
    std::cout << "                 系统日志" << std::endl;
    std::cout << "=============================================" << std::endl;
    if (transactions.empty()) {
        std::cout << "暂无交易记录" << std::endl;
    } else {
        for (const auto& trans : transactions) {
            std::cout << format_time(trans.timestamp) << " "
                 << "用户: " << trans.user_id << " "
                 << (trans.type == "buy" ? "购买" : "进货") << " "
                 << trans.isbn << " "
                 << "数量: " << trans.quantity << " "
                 << "总价: " << format_double(trans.total) << std::endl;
        }
    }
    std::cout << "=============================================" << std::endl;
}
// Created by Lenovo on 2025/12/13.
//
