#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace std;

const int MAX_KEY_LEN = 64;
const int CACHE_SIZE = 300; // Keep a cache to reduce file I/O

struct Entry {
    char key[MAX_KEY_LEN + 1];
    int value;
    
    Entry() : value(0) {
        memset(key, 0, sizeof(key));
    }
    
    Entry(const char* k, int v) : value(v) {
        memset(key, 0, sizeof(key));
        strncpy(key, k, MAX_KEY_LEN);
    }
    
    bool operator<(const Entry& other) const {
        int cmp = strcmp(key, other.key);
        if (cmp != 0) return cmp < 0;
        return value < other.value;
    }
    
    bool operator==(const Entry& other) const {
        return strcmp(key, other.key) == 0 && value == other.value;
    }
    
    bool same_key(const char* k) const {
        return strcmp(key, k) == 0;
    }
};

class FileStorage {
private:
    const char* filename = "storage.dat";
    vector<Entry> cache;
    
    vector<Entry> load_all() {
        vector<Entry> all_entries(cache);
        
        ifstream file(filename, ios::binary);
        if (file.is_open()) {
            Entry entry;
            while (file.read((char*)&entry, sizeof(Entry))) {
                all_entries.push_back(entry);
            }
            file.close();
        }
        
        return all_entries;
    }
    
    void save_all(const vector<Entry>& entries) {
        ofstream file(filename, ios::binary | ios::trunc);
        for (const auto& entry : entries) {
            file.write((const char*)&entry, sizeof(Entry));
        }
        file.close();
        cache.clear();
    }
    
    void flush_cache() {
        if (cache.empty()) return;
        
        vector<Entry> all_entries = load_all();
        sort(all_entries.begin(), all_entries.end());
        save_all(all_entries);
    }
    
public:
    FileStorage() {}
    
    ~FileStorage() {
        flush_cache();
    }
    
    void insert(const char* key, int value) {
        Entry new_entry(key, value);
        cache.push_back(new_entry);
        
        if (cache.size() >= CACHE_SIZE) {
            flush_cache();
        }
    }
    
    void remove(const char* key, int value) {
        flush_cache();
        
        vector<Entry> all_entries = load_all();
        Entry target(key, value);
        
        auto it = std::find(all_entries.begin(), all_entries.end(), target);
        if (it != all_entries.end()) {
            all_entries.erase(it);
            save_all(all_entries);
        }
    }
    
    vector<int> find(const char* key) {
        vector<int> result;
        
        vector<Entry> all_entries = load_all();
        
        for (const auto& entry : all_entries) {
            if (entry.same_key(key)) {
                result.push_back(entry.value);
            }
        }
        
        sort(result.begin(), result.end());
        return result;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    FileStorage storage;
    
    int n;
    cin >> n;
    
    for (int i = 0; i < n; i++) {
        string cmd;
        cin >> cmd;
        
        if (cmd == "insert") {
            string index;
            int value;
            cin >> index >> value;
            storage.insert(index.c_str(), value);
        } else if (cmd == "delete") {
            string index;
            int value;
            cin >> index >> value;
            storage.remove(index.c_str(), value);
        } else if (cmd == "find") {
            string index;
            cin >> index;
            vector<int> results = storage.find(index.c_str());
            
            if (results.empty()) {
                cout << "null\n";
            } else {
                for (size_t j = 0; j < results.size(); j++) {
                    if (j > 0) cout << " ";
                    cout << results[j];
                }
                cout << "\n";
            }
        }
    }
    
    return 0;
}
