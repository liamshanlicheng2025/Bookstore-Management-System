#ifndef BLOCKLISTDB_H
#define BLOCKLISTDB_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
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

class BlockListDB {
private:
    string filename;
    map<string, string> data_cache;
    bool loaded;

    void ensure_directory(const string& path) {
        size_t pos = 0;
        string dir;

        while ((pos = path.find_first_of("/\\", pos + 1)) != string::npos) {
            dir = path.substr(0, pos);
            struct stat st;
            if (stat(dir.c_str(), &st) != 0) {
                MKDIR(dir.c_str());
            }
        }
    }

    void load_data() {
        if (loaded) return;

        data_cache.clear();
        ifstream file(filename);
        if (!file.is_open()) {
            // 文件不存在，创建空缓存
            loaded = true;
            return;
        }

        string line;
        while (getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != string::npos) {
                string key = line.substr(0, pos);
                string value = line.substr(pos + 1);
                data_cache[key] = value;
            }
        }
        file.close();
        loaded = true;
    }

    void save_data() {
        ofstream file(filename);
        if (!file.is_open()) {
            return;
        }

        for (const auto& entry : data_cache) {
            file << entry.first << "=" << entry.second << "\n";
        }
        file.close();
    }

public:
    BlockListDB(const string& fname) : filename(fname), loaded(false) {
        ensure_directory(filename);
    }

    bool insert(const string& key, const string& value) {
        load_data();

        if (data_cache.find(key) != data_cache.end()) {
            return false; // 键已存在
        }

        data_cache[key] = value;
        save_data();
        return true;
    }

    bool update(const string& key, const string& value) {
        load_data();

        if (data_cache.find(key) == data_cache.end()) {
            return false; // 键不存在
        }

        data_cache[key] = value;
        save_data();
        return true;
    }

    bool remove(const string& key) {
        load_data();

        if (data_cache.find(key) == data_cache.end()) {
            return false; // 键不存在
        }

        data_cache.erase(key);
        save_data();
        return true;
    }

    string find(const string& key) {
        load_data();

        auto it = data_cache.find(key);
        if (it != data_cache.end()) {
            return it->second;
        }
        return "";
    }

    vector<pair<string, string>> find_all() {
        load_data();

        vector<pair<string, string>> result;
        for (const auto& entry : data_cache) {
            result.push_back(entry);
        }
        return result;
    }

    vector<pair<string, string>> find_prefix(const string& prefix) {
        load_data();

        vector<pair<string, string>> result;
        for (const auto& entry : data_cache) {
            if (entry.first.find(prefix) == 0) {
                result.push_back(entry);
            }
        }
        return result;
    }

    bool insert_or_update(const string& key, const string& value) {
        load_data();

        data_cache[key] = value;
        save_data();
        return true;
    }
};

#endif // BLOCKLISTDB_H
//DONE: 修改"文件存储测试.cpp"适应书店需求
// 应该需要加模板
