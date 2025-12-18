#ifndef BLOCKLISTDB_H
#define BLOCKLISTDB_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(dir) _mkdir(dir)
#else
#include <sys/stat.h>
#define MKDIR(dir) mkdir(dir, 0755)
#endif

using namespace std;

const int BLOCK_SIZE = 100;

struct SimpleRecord {
    char key[65];
    char value[256];
    SimpleRecord() {
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
    }
    SimpleRecord(const string& k, const string& v) {
        strncpy(key, k.c_str(), 64);
        key[64] = '\0';
        strncpy(value, v.c_str(), 255);
        value[255] = '\0';
    }
};

struct SimpleBlock {
    int record_count;
    int next_block;
    SimpleRecord records[BLOCK_SIZE];
    SimpleBlock() : record_count(0), next_block(-1) {}
};

class BlockListDB {
private:
    string data_file;
    fstream file;
    int first_block_pos;

    // 确保目录存在
    void ensure_directory(const string& filename) {
        size_t last_slash = filename.find_last_of("/\\");
        if (last_slash != string::npos) {
            string dir = filename.substr(0, last_slash);
            struct stat st;
            if (stat(dir.c_str(), &st) != 0) {
                MKDIR(dir.c_str());
            }
        }
    }

    // 获取文件末尾位置
    int get_file_end() {
        file.seekg(0, ios::end);
        return file.tellg();
    }

    // 读取块
    SimpleBlock read_block(int pos) {
        SimpleBlock block;
        if (pos < 0) return block;
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&block.record_count), sizeof(int));
        file.read(reinterpret_cast<char*>(&block.next_block), sizeof(int));
        for (int i = 0; i < block.record_count; i++) {
            file.read(block.records[i].key, 65);
            file.read(block.records[i].value, 256);
        }
        return block;
    }

    // 写入块
    void write_block(int pos, const SimpleBlock& block) {
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(&block.record_count), sizeof(int));
        file.write(reinterpret_cast<const char*>(&block.next_block), sizeof(int));
        for (int i = 0; i < block.record_count; i++) {
            file.write(block.records[i].key, 65);
            file.write(block.records[i].value, 256);
        }
        file.flush();
    }

    int find_record_in_block(const SimpleBlock& block, const string& key) {
        for (int i = 0; i < block.record_count; i++) {
            if (string(block.records[i].key) == key) {
                return i;
            }
        }
        return -1;
    }

public:
    BlockListDB(const string& filename) : data_file(filename), first_block_pos(-1) {
        ensure_directory(filename);

        // 尝试打开文件
        file.open(filename, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            // 创建新文件
            file.open(filename, ios::out | ios::binary);
            if (!file.is_open()) {
                cerr << "Error: Cannot create file " << filename << endl;
                return;
            }
            file.close();

            // 重新以读写模式打开
            file.open(filename, ios::in | ios::out | ios::binary);
            if (!file.is_open()) {
                cerr << "Error: Cannot open file " << filename << " for reading and writing" << endl;
                return;
            }

            first_block_pos = 0;
            SimpleBlock first_block;
            write_block(first_block_pos, first_block);

            // 写入first_block_pos到文件开头
            file.seekp(0, ios::beg);
            file.write(reinterpret_cast<const char*>(&first_block_pos), sizeof(int));
            file.flush();
        } else {
            // 读取first_block_pos
            file.seekg(0, ios::beg);
            if (file.peek() == EOF) {
                // 文件为空，初始化
                first_block_pos = 0;
                SimpleBlock first_block;
                write_block(first_block_pos, first_block);
                file.seekp(0, ios::beg);
                file.write(reinterpret_cast<const char*>(&first_block_pos), sizeof(int));
                file.flush();
            } else {
                file.read(reinterpret_cast<char*>(&first_block_pos), sizeof(int));
            }
        }
    }

    ~BlockListDB() {
        if (file.is_open()) {
            file.close();
        }
    }

    // 检查数据库是否有效
    bool is_valid() const {
        return first_block_pos != -1 && file.is_open();
    }

    // 插入记录
    bool insert(const string& key, const string& value) {
        if (!is_valid() || key.empty() || key.length() > 64) {
            return false;
        }

        // 检查键是否已存在
        if (!find(key).empty()) {
            return false;
        }

        int current_pos = first_block_pos;
        while (true) {
            SimpleBlock block = read_block(current_pos);
            if (block.record_count < BLOCK_SIZE) {
                // 块中有空间
                block.records[block.record_count] = SimpleRecord(key, value);
                block.record_count++;
                write_block(current_pos, block);
                return true;
            }

            if (block.next_block == -1) {
                // 创建新块
                int new_pos = get_file_end();
                SimpleBlock new_block;
                new_block.records[0] = SimpleRecord(key, value);
                new_block.record_count = 1;
                block.next_block = new_pos;
                write_block(current_pos, block);
                write_block(new_pos, new_block);
                return true;
            }

            current_pos = block.next_block;
        }
    }

    // 更新记录
    bool update(const string& key, const string& value) {
        if (!is_valid() || key.empty()) return false;

        int current_pos = first_block_pos;
        while (current_pos != -1) {
            SimpleBlock block = read_block(current_pos);
            for (int i = 0; i < block.record_count; i++) {
                if (string(block.records[i].key) == key) {
                    strncpy(block.records[i].value, value.c_str(), 255);
                    block.records[i].value[255] = '\0';
                    write_block(current_pos, block);
                    return true;
                }
            }
            current_pos = block.next_block;
        }
        return false;
    }

    // 删除记录
    bool remove(const string& key) {
        if (!is_valid() || key.empty()) return false;

        int current_pos = first_block_pos;
        while (current_pos != -1) {
            SimpleBlock block = read_block(current_pos);
            for (int i = 0; i < block.record_count; i++) {
                if (string(block.records[i].key) == key) {
                    // 移动后续记录
                    for (int j = i; j < block.record_count - 1; j++) {
                        block.records[j] = block.records[j + 1];
                    }
                    block.record_count--;
                    write_block(current_pos, block);
                    return true;
                }
            }
            current_pos = block.next_block;
        }
        return false;
    }

    // 查找记录
    string find(const string& key) {
        if (!is_valid() || key.empty()) return "";

        int current_pos = first_block_pos;
        while (current_pos != -1) {
            SimpleBlock block = read_block(current_pos);
            for (int i = 0; i < block.record_count; i++) {
                if (string(block.records[i].key) == key) {
                    return string(block.records[i].value);
                }
            }
            current_pos = block.next_block;
        }
        return "";
    }

    // 查找所有记录
    vector<pair<string, string>> find_all() {
        vector<pair<string, string>> result;
        if (!is_valid()) return result;

        int current_pos = first_block_pos;
        while (current_pos != -1) {
            SimpleBlock block = read_block(current_pos);
            for (int i = 0; i < block.record_count; i++) {
                result.push_back({string(block.records[i].key),
                                  string(block.records[i].value)});
            }
            current_pos = block.next_block;
        }
        return result;
    }

    // 查找前缀匹配的记录
    vector<pair<string, string>> find_prefix(const string& prefix) {
        vector<pair<string, string>> result;
        if (!is_valid()) return result;

        auto all = find_all();
        for (const auto& entry : all) {
            if (entry.first.find(prefix) == 0) {
                result.push_back(entry);
            }
        }
        return result;
    }
};

#endif // BLOCKLISTDB_H
//DONE: 修改"文件存储测试.cpp"适应书店需求
// 应该需要加模板
