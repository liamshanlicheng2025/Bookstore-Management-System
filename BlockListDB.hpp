#ifndef BLOCKLISTDB_H
#define BLOCKLISTDB_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <map>

using namespace std;

const int BLOCK_SIZE = 16;
const int INDEX_SIZE = 65;
const int VALUE_SIZE = 1024;

struct Record {
    char index[INDEX_SIZE];
    char value[VALUE_SIZE];

    Record() {
        memset(index, 0, sizeof(index));
        memset(value, 0, sizeof(value));
    }

    Record(const string& idx, const string& val) {
        strncpy(index, idx.c_str(), INDEX_SIZE - 1);
        index[INDEX_SIZE - 1] = '\0';
        strncpy(value, val.c_str(), VALUE_SIZE - 1);
        value[VALUE_SIZE - 1] = '\0';
    }

    bool operator<(const Record& other) const {
        int cmp = strcmp(index, other.index);
        if (cmp != 0) return cmp < 0;
        return strcmp(value, other.value) < 0;
    }

    bool operator==(const Record& other) const {
        return strcmp(index, other.index) == 0 && strcmp(value, other.value) == 0;
    }

    string get_index() const { return string(index); }
    string get_value() const { return string(value); }
};

struct Block {
    int record_count;
    int next_block;
    char first_index[INDEX_SIZE];
    char last_index[INDEX_SIZE];
    Record records[BLOCK_SIZE];

    Block() : record_count(0), next_block(-1) {
        memset(first_index, 0, sizeof(first_index));
        memset(last_index, 0, sizeof(last_index));
    }

    void update_boundaries() {
        if (record_count > 0) {
            strcpy(first_index, records[0].index);
            strcpy(last_index, records[record_count - 1].index);
        } else {
            memset(first_index, 0, sizeof(first_index));
            memset(last_index, 0, sizeof(last_index));
        }
    }
};

struct BlockIndexEntry {
    int block_offset;
    char first_index[INDEX_SIZE];
    char last_index[INDEX_SIZE];

    BlockIndexEntry() : block_offset(-1) {
        memset(first_index, 0, sizeof(first_index));
        memset(last_index, 0, sizeof(last_index));
    }
};

class BlockListDB {
private:
    string filename;
    fstream data_file;
    int first_block_offset;
    vector<BlockIndexEntry> block_index;

    Block read_block(int offset) {
        Block block;
        if (offset < 0) return block;

        data_file.seekg(offset);
        data_file.read(reinterpret_cast<char*>(&block.record_count), sizeof(int));
        data_file.read(reinterpret_cast<char*>(&block.next_block), sizeof(int));
        data_file.read(block.first_index, INDEX_SIZE);
        data_file.read(block.last_index, INDEX_SIZE);
        data_file.read(reinterpret_cast<char*>(block.records), sizeof(Record) * BLOCK_SIZE);

        return block;
    }

    void write_block(int offset, const Block& block) {
        data_file.seekp(offset);
        data_file.write(reinterpret_cast<const char*>(&block.record_count), sizeof(int));
        data_file.write(reinterpret_cast<const char*>(&block.next_block), sizeof(int));
        data_file.write(block.first_index, INDEX_SIZE);
        data_file.write(block.last_index, INDEX_SIZE);
        data_file.write(reinterpret_cast<const char*>(block.records), sizeof(Record) * BLOCK_SIZE);
        data_file.flush();
    }

    int get_end_position() {
        data_file.seekg(0, ios::end);
        return data_file.tellg();
    }

    int create_new_block() {
        Block new_block;
        int pos = get_end_position();
        write_block(pos, new_block);

        BlockIndexEntry entry;
        entry.block_offset = pos;
        block_index.push_back(entry);

        return pos;
    }

    void load_block_index() {
        block_index.clear();
        int current_offset = first_block_offset;

        while (current_offset != -1) {
            Block block = read_block(current_offset);
            BlockIndexEntry entry;
            entry.block_offset = current_offset;
            strcpy(entry.first_index, block.first_index);
            strcpy(entry.last_index, block.last_index);
            block_index.push_back(entry);
            current_offset = block.next_block;
        }
    }

    void save_metadata() {
        data_file.seekp(0);
        data_file.write(reinterpret_cast<const char*>(&first_block_offset), sizeof(int));
    }

    void load_metadata() {
        data_file.seekg(0);
        if (data_file.peek() == EOF) {
            first_block_offset = create_new_block();
            save_metadata();
        } else {
            data_file.read(reinterpret_cast<char*>(&first_block_offset), sizeof(int));
        }
    }

    int find_insert_position(const Block& block, const string& index) {
        int left = 0, right = block.record_count - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            int cmp = strcmp(block.records[mid].index, index.c_str());

            if (cmp == 0) {
                return -1; // 键已存在
            }
            if (cmp < 0) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return left; // 插入位置
    }

    void split_block(int block_offset, Block& block) {
        int new_block_offset = create_new_block();
        Block new_block;

        int mid = block.record_count / 2;
        for (int i = mid; i < block.record_count; i++) {
            new_block.records[new_block.record_count++] = block.records[i];
        }
        block.record_count = mid;

        block.update_boundaries();
        new_block.update_boundaries();

        new_block.next_block = block.next_block;
        block.next_block = new_block_offset;

        write_block(block_offset, block);
        write_block(new_block_offset, new_block);

        for (auto& entry : block_index) {
            if (entry.block_offset == block_offset) {
                strcpy(entry.first_index, block.first_index);
                strcpy(entry.last_index, block.last_index);
            }
        }

        BlockIndexEntry new_entry;
        new_entry.block_offset = new_block_offset;
        strcpy(new_entry.first_index, new_block.first_index);
        strcpy(new_entry.last_index, new_block.last_index);

        for (auto it = block_index.begin(); it != block_index.end(); ++it) {
            if (it->block_offset == block_offset) {
                block_index.insert(it + 1, new_entry);
                break;
            }
        }
    }

    int find_block_for_insert(const string& index) {
        if (block_index.empty()) return first_block_offset;

        for (const auto& entry : block_index) {
            if (entry.block_offset == -1) continue;
            Block block = read_block(entry.block_offset);

            if (block.record_count == 0) {
                return entry.block_offset;
            }

            if (strcmp(index.c_str(), entry.last_index) <= 0 || (block.next_block == -1)) {
                return entry.block_offset;
            }
        }

        return block_index.back().block_offset;
    }

    vector<pair<int, int>> find_record_positions(const string& index) {
        vector<pair<int, int>> positions;

        for (const auto& entry : block_index) {
            if (entry.block_offset == -1) continue;
            if (strcmp(index.c_str(), entry.first_index) < 0) {
                break;
            }
            if (strcmp(index.c_str(), entry.last_index) > 0) {
                continue;
            }

            Block block = read_block(entry.block_offset);

            int left = 0, right = block.record_count - 1;
            int first_pos = -1;

            while (left <= right) {
                int mid = left + (right - left) / 2;
                int cmp = strcmp(block.records[mid].index, index.c_str());

                if (cmp == 0) {
                    first_pos = mid;
                    right = mid - 1;
                } else if (cmp < 0) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }

            if (first_pos != -1) {
                for (int i = first_pos; i < block.record_count; i++) {
                    if (strcmp(block.records[i].index, index.c_str()) == 0) {
                        positions.push_back({entry.block_offset, i});
                    } else {
                        break;
                    }
                }
            }
        }

        return positions;
    }

public:
    BlockListDB(const string& fname) : filename(fname), first_block_offset(-1) {
        bool data_exists = false;
        ifstream test(filename);
        if (test.good()) {
            data_exists = true;
        }
        test.close();

        if (!data_exists) {
            data_file.open(filename, ios::in | ios::out | ios::binary | ios::trunc);
            data_file.seekp(sizeof(int) - 1);
            data_file.put(0);
            first_block_offset = create_new_block();
            save_metadata();
        } else {
            data_file.open(filename, ios::in | ios::out | ios::binary);
            load_metadata();
            load_block_index();
        }
    }

    ~BlockListDB() {
        if (data_file.is_open()) {
            data_file.close();
        }
    }

    bool insert(const string& key, const string& value) {
        // 检查键是否已存在（书店系统中键应该是唯一的）
        if (!find(key).empty()) {
            return false;
        }

        Record new_record(key, value);

        int block_offset = find_block_for_insert(key);
        Block block = read_block(block_offset);

        int pos = find_insert_position(block, key);
        if (pos == -1) {
            return false;
        }

        for (int i = block.record_count; i > pos; i--) {
            block.records[i] = block.records[i - 1];
        }

        block.records[pos] = new_record;
        block.record_count++;
        block.update_boundaries();

        if (block.record_count >= BLOCK_SIZE) {
            write_block(block_offset, block);
            split_block(block_offset, block);
        } else {
            write_block(block_offset, block);

            for (auto& entry : block_index) {
                if (entry.block_offset == block_offset) {
                    strcpy(entry.first_index, block.first_index);
                    strcpy(entry.last_index, block.last_index);
                    break;
                }
            }
        }

        return true;
    }

    bool update(const string& key, const string& value) {
        // 先删除所有该键的记录
        bool removed = remove(key);
        if (!removed) {
            // 如果键不存在，就插入新记录
            return insert(key, value);
        }
        // 插入新值
        return insert(key, value);
    }

    bool remove(const string& key) {
        auto positions = find_record_positions(key);
        if (positions.empty()) {
            return false;
        }

        // 从后往前删除，避免索引变化
        sort(positions.rbegin(), positions.rend());

        for (const auto& pos : positions) {
            int block_offset = pos.first;
            int record_index = pos.second;

            Block block = read_block(block_offset);

            for (int i = record_index; i < block.record_count - 1; i++) {
                block.records[i] = block.records[i + 1];
            }
            block.record_count--;
            block.update_boundaries();
            write_block(block_offset, block);

            for (auto& entry : block_index) {
                if (entry.block_offset == block_offset) {
                    strcpy(entry.first_index, block.first_index);
                    strcpy(entry.last_index, block.last_index);
                    break;
                }
            }
        }

        return true;
    }

    string find(const string& key) {
        auto positions = find_record_positions(key);
        if (!positions.empty()) {
            int block_offset = positions[0].first;
            int record_index = positions[0].second;
            Block block = read_block(block_offset);
            return block.records[record_index].get_value();
        }
        return "";
    }

    vector<pair<string, string>> find_all() {
        vector<pair<string, string>> result;

        for (const auto& entry : block_index) {
            if (entry.block_offset == -1) continue;

            Block block = read_block(entry.block_offset);
            for (int i = 0; i < block.record_count; i++) {
                result.push_back(make_pair(
                        block.records[i].get_index(),
                        block.records[i].get_value()
                ));
            }
        }

        return result;
    }

    vector<pair<string, string>> find_prefix(const string& prefix) {
        vector<pair<string, string>> result;

        for (const auto& entry : block_index) {
            if (entry.block_offset == -1) continue;

            if (strncmp(prefix.c_str(), entry.first_index, prefix.length()) > 0) {
                continue;
            }
            if (strncmp(prefix.c_str(), entry.last_index, prefix.length()) < 0) {
                break;
            }

            Block block = read_block(entry.block_offset);
            for (int i = 0; i < block.record_count; i++) {
                if (strncmp(block.records[i].index, prefix.c_str(), prefix.length()) == 0) {
                    result.push_back(make_pair(
                            block.records[i].get_index(),
                            block.records[i].get_value()
                    ));
                }
            }
        }

        return result;
    }

    bool insert_or_update(const string& key, const string& value) {
        if (!find(key).empty()) {
            return update(key, value);
        }
        return insert(key, value);
    }
};

#endif // BLOCKLISTDB_H