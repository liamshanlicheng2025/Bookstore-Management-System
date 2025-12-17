#ifndef BLOCKLIST_H
#define BLOCKLIST_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#endif

using namespace std;

const int BLOCK_SIZE = 100;

struct Record {
    char key[65];
    char value[256];
    bool active;
    Record() {
        memset(key, 0, sizeof(key));
        memset(value, 0, sizeof(value));
        active = true;
    }
    Record(const string& k, const string& v) {
        strncpy(key, k.c_str(), 64);
        strncpy(value, v.c_str(), 255);
        key[64] = '\0';
        value[255] = '\0';
        active = true;
    }
};

struct Block {
    int record_count;
    int next_block;
    Record records[BLOCK_SIZE];
    Block() : record_count(0), next_block(-1) {}
};

class BlockListDB {
private:
    string data_file;
    fstream file;
    int first_block_pos;

    int get_file_end() {
        file.seekg(0, ios::end);
        return file.tellg();
    }

    Block read_block(int pos) {
        Block block;
        if (pos < 0) return block;
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&block.record_count), sizeof(int));
        file.read(reinterpret_cast<char*>(&block.next_block), sizeof(int));
        for (int i = 0; i < block.record_count; i++){
            file.read(block.records[i].key, 65);
            file.read(block.records[i].value, 256);
            file.read(reinterpret_cast<char*>(&block.records[i].active), sizeof(bool));
        }
        return block;
    }

    void write_block(int pos, const Block& block) {
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(&block.record_count), sizeof(int));
        file.write(reinterpret_cast<const char*>(&block.next_block), sizeof(int));
        for (int i = 0; i < block.record_count; i++){
            file.write(block.records[i].key, 65);
            file.write(block.records[i].value, 256);
            file.write(reinterpret_cast<const char*>(&block.records[i].active), sizeof(bool));
        }
        file.flush();
    }

    int find_record_in_block(const Block& block, const string& key){
        for (int i = 0; i < block.record_count; i++){
            if (block.records[i].active && string(block.records[i].key) == key){
                return i;
            }
        }
        return -1;
    }

public:
    BlockListDB(const string& filename) : data_file(filename), first_block_pos(-1) {
        // 确保目录存在
        size_t last_slash = filename.find_last_of("/\\");
        if (last_slash != string::npos) {
            string dir = filename.substr(0, last_slash);
            struct stat st;
            if (stat(dir.c_str(), &st) != 0) {
                mkdir(dir.c_str(), 0755);
            }
        }

        file.open(filename, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            // 创建新文件
            file.open(filename, ios::out | ios::binary);
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);
            first_block_pos = 0;
            Block first_block;
            write_block(first_block_pos, first_block);
            file.seekp(0, ios::beg);
            file.write(reinterpret_cast<const char*>(&first_block_pos), sizeof(int));
            file.flush();
        } else {
            file.seekg(0, ios::beg);
            // 检查文件是否有内容
            if (file.peek() == EOF) {
                first_block_pos = 0;
                Block first_block;
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

    bool insert(const string& key, const string& value) {
        if (key.empty() || key.length() > 64) return false;
        if (first_block_pos == -1) return false;

        int current_pos = first_block_pos;
        Block block;

        // 检查是否已存在相同的key
        while (current_pos != -1){
            block = read_block(current_pos);
            if (find_record_in_block(block, key) != -1) return false;
            current_pos = block.next_block;
        }

        // 在块中查找插入位置
        current_pos = first_block_pos;
        while (true){
            block = read_block(current_pos);
            if (block.record_count < BLOCK_SIZE){
                block.records[block.record_count] = Record(key, value);
                block.record_count++;
                write_block(current_pos, block);
                return true;
            }

            // 如果块满了且后面没有块，创建一个新的块
            if (block.next_block == -1){
                int new_pos = get_file_end();
                Block new_block;
                new_block.records[0] = Record(key, value);
                new_block.record_count = 1;
                block.next_block = new_pos;
                write_block(current_pos, block);
                write_block(new_pos, new_block);
                return true;
            }
            current_pos = block.next_block;
        }
    }

    bool update(const string& key, const string& value){
        if (key.empty() || first_block_pos == -1) return false;

        int current_pos = first_block_pos;
        while (current_pos != -1){
            Block block = read_block(current_pos);
            for (int i = 0; i < block.record_count; i++){
                if (block.records[i].active && string(block.records[i].key) == key){
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

    bool remove(const string& key) {
        if (key.empty() || first_block_pos == -1) return false;

        int current_pos = first_block_pos;
        while (current_pos != -1){
            Block block = read_block(current_pos);
            for (int i = 0; i < block.record_count; i++){
                if (block.records[i].active && string(block.records[i].key) == key){
                    block.records[i].active = false;
                    write_block(current_pos, block);
                    return true;
                }
            }
            current_pos = block.next_block;
        }
        return false;
    }

    string find(const string& key) {
        if (key.empty() || first_block_pos == -1) return "";

        int current_pos = first_block_pos;
        while (current_pos != -1){
            Block block = read_block(current_pos);
            for (int i = 0; i < block.record_count; i++){
                if (block.records[i].active && string(block.records[i].key) == key){
                    return string(block.records[i].value);
                }
            }
            current_pos = block.next_block;
        }
        return "";
    }

    vector<pair<string, string>> find_all(){
        vector<pair<string, string>> result;
        int current_pos = first_block_pos;
        while (current_pos != -1){
            Block block = read_block(current_pos);
            for (int i = 0; i < block.record_count; i++){
                if (block.records[i].active){
                    result.push_back({string(block.records[i].key), string(block.records[i].value)});
                }
            }
            current_pos = block.next_block;
        }
        return result;
    }

    vector<pair<string, string>> find_prefix(const string& prefix){
        vector<pair<string, string>> result;
        auto all = find_all();
        for (const auto& entry : all){
            if (entry.first.find(prefix) == 0){
                result.push_back(entry);
            }
        }
        return result;
    }
};

#endif
//DONE: 修改"文件存储测试.cpp"适应书店需求
// 应该需要加模板
