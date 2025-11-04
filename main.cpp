#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace std;

const int MAX_KEY_LEN = 64;
const int BUFFER_SIZE = 500; // Buffer writes before flushing

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
    
    void flush() {
        if (buffer.empty()) return;
        
        // Load existing data
        vector<Entry> all_data;
        ifstream fin(filename, ios::binary);
        if (fin) {
            Entry entry;
            while (fin.read((char*)&entry, sizeof(Entry))) {
                all_data.push_back(entry);
            }
            fin.close();
        }
        
        // Merge with buffer
        for (const auto& e : buffer) {
            all_data.push_back(e);
        }
        buffer.clear();
        
        // Sort and write back
        sort(all_data.begin(), all_data.end());
        
        ofstream fout(filename, ios::binary | ios::trunc);
        for (const auto& entry : all_data) {
            fout.write((const char*)&entry, sizeof(Entry));
        }
        fout.close();
    }
    
public:
    FileStorage() {}
    
    ~FileStorage() {
        flush();
    }
    
    void insert(const char* key, int value) {
        buffer.push_back(Entry(key, value));
        
        if (buffer.size() >= BUFFER_SIZE) {
            flush();
        }
    }
    
    void remove(const char* key, int value) {
        flush(); // Ensure all data is on disk
        
        Entry target(key, value);
        vector<Entry> all_data;
        
        ifstream fin(filename, ios::binary);
        if (fin) {
            Entry entry;
            while (fin.read((char*)&entry, sizeof(Entry))) {
                if (!(entry == target)) {
                    all_data.push_back(entry);
                }
            }
            fin.close();
        }
        
        ofstream fout(filename, ios::binary | ios::trunc);
        for (const auto& entry : all_data) {
            fout.write((const char*)&entry, sizeof(Entry));
        }
        fout.close();
    }
    
    vector<int> find(const char* key) {
        flush(); // Ensure all data is on disk
        
        vector<int> result;
        
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
