#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cstdio>

using namespace std;

const int MAX_KEY_LEN = 64;
const int CHUNK_SIZE = 800; // Balance between memory and performance

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
    vector<Entry> buffer;
    
    void flush_buffer() {
        if (buffer.empty()) return;
        
        // Append buffer to file (unsorted)
        ofstream fout(filename, ios::binary | ios::app);
        for (const auto& entry : buffer) {
            fout.write((const char*)&entry, sizeof(Entry));
        }
        fout.close();
        buffer.clear();
    }
    
    void sort_file() {
        flush_buffer();
        
        // Read all and sort
        vector<Entry> all_data;
        ifstream fin(filename, ios::binary);
        if (fin) {
            Entry entry;
            while (fin.read((char*)&entry, sizeof(Entry))) {
                all_data.push_back(entry);
            }
            fin.close();
        }
        
        sort(all_data.begin(), all_data.end());
        
        // Write back
        ofstream fout(filename, ios::binary | ios::trunc);
        for (const auto& entry : all_data) {
            fout.write((const char*)&entry, sizeof(Entry));
        }
        fout.close();
    }
    
public:
    FileStorage() {}
    
    ~FileStorage() {
        flush_buffer();
    }
    
    void insert(const char* key, int value) {
        buffer.push_back(Entry(key, value));
        
        if (buffer.size() >= CHUNK_SIZE) {
            flush_buffer();
        }
    }
    
    void remove(const char* key, int value) {
        flush_buffer();
        
        Entry target(key, value);
        const char* temp_file = "temp.dat";
        
        ifstream fin(filename, ios::binary);
        ofstream fout(temp_file, ios::binary);
        
        if (fin) {
            Entry entry;
            while (fin.read((char*)&entry, sizeof(Entry))) {
                if (!(entry == target)) {
                    fout.write((const char*)&entry, sizeof(Entry));
                }
            }
            fin.close();
        }
        fout.close();
        
        std::remove(filename);
        rename(temp_file, filename);
    }
    
    vector<int> find(const char* key) {
        vector<int> result;
        
        // Search buffer
        for (const auto& entry : buffer) {
            if (entry.same_key(key)) {
                result.push_back(entry.value);
            }
        }
        
        // Search file
        ifstream fin(filename, ios::binary);
        if (fin) {
            Entry entry;
            while (fin.read((char*)&entry, sizeof(Entry))) {
                if (entry.same_key(key)) {
                    result.push_back(entry.value);
                }
            }
            fin.close();
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
