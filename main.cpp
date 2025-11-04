#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace std;

const int MAX_KEY_LEN = 64;
const int BLOCK_SIZE = 4096;
const int ENTRIES_PER_BLOCK = BLOCK_SIZE / (MAX_KEY_LEN + 1 + 4 + 8); // ~50 entries

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

struct Block {
    int count;
    Entry entries[ENTRIES_PER_BLOCK];
    
    Block() : count(0) {}
};

class FileStorage {
private:
    const char* filename = "storage.dat";
    fstream file;
    int num_blocks;
    
    void open_file() {
        file.open(filename, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            file.open(filename, ios::out | ios::binary);
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);
            num_blocks = 0;
        } else {
            file.seekg(0, ios::end);
            streampos size = file.tellg();
            num_blocks = size / sizeof(Block);
        }
    }
    
    void read_block(int idx, Block& block) {
        file.seekg(idx * sizeof(Block));
        file.read((char*)&block, sizeof(Block));
    }
    
    void write_block(int idx, const Block& block) {
        file.seekp(idx * sizeof(Block));
        file.write((const char*)&block, sizeof(Block));
        file.flush();
    }
    
    void rebuild() {
        vector<Entry> all_entries;
        
        for (int i = 0; i < num_blocks; i++) {
            Block block;
            read_block(i, block);
            for (int j = 0; j < block.count; j++) {
                all_entries.push_back(block.entries[j]);
            }
        }
        
        sort(all_entries.begin(), all_entries.end());
        
        file.close();
        file.open(filename, ios::out | ios::binary | ios::trunc);
        file.close();
        file.open(filename, ios::in | ios::out | ios::binary);
        
        num_blocks = (all_entries.size() + ENTRIES_PER_BLOCK - 1) / ENTRIES_PER_BLOCK;
        
        for (int i = 0; i < num_blocks; i++) {
            Block block;
            int start = i * ENTRIES_PER_BLOCK;
            int end = min((i + 1) * ENTRIES_PER_BLOCK, (int)all_entries.size());
            block.count = end - start;
            
            for (int j = 0; j < block.count; j++) {
                block.entries[j] = all_entries[start + j];
            }
            
            write_block(i, block);
        }
    }
    
public:
    FileStorage() {
        open_file();
    }
    
    ~FileStorage() {
        if (file.is_open()) {
            file.close();
        }
    }
    
    void insert(const char* key, int value) {
        Entry new_entry(key, value);
        
        for (int i = 0; i < num_blocks; i++) {
            Block block;
            read_block(i, block);
            
            if (block.count < ENTRIES_PER_BLOCK) {
                int j = block.count - 1;
                while (j >= 0 && new_entry < block.entries[j]) {
                    block.entries[j + 1] = block.entries[j];
                    j--;
                }
                block.entries[j + 1] = new_entry;
                block.count++;
                write_block(i, block);
                return;
            } else if (i == num_blocks - 1) {
                Block new_block;
                new_block.count = 1;
                new_block.entries[0] = new_entry;
                write_block(num_blocks, new_block);
                num_blocks++;
                rebuild();
                return;
            }
        }
        
        if (num_blocks == 0) {
            Block block;
            block.count = 1;
            block.entries[0] = new_entry;
            write_block(0, block);
            num_blocks = 1;
        }
    }
    
    void remove(const char* key, int value) {
        Entry target(key, value);
        
        for (int i = 0; i < num_blocks; i++) {
            Block block;
            read_block(i, block);
            
            for (int j = 0; j < block.count; j++) {
                if (block.entries[j] == target) {
                    for (int k = j; k < block.count - 1; k++) {
                        block.entries[k] = block.entries[k + 1];
                    }
                    block.count--;
                    write_block(i, block);
                    return;
                }
            }
        }
    }
    
    vector<int> find(const char* key) {
        vector<int> result;
        
        for (int i = 0; i < num_blocks; i++) {
            Block block;
            read_block(i, block);
            
            for (int j = 0; j < block.count; j++) {
                if (block.entries[j].same_key(key)) {
                    result.push_back(block.entries[j].value);
                }
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
