#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace std;

const int MAX_KEY_LEN = 64;
const int BLOCK_SIZE = 4096;
const int MAX_PAIRS = (BLOCK_SIZE - 16) / (MAX_KEY_LEN + 8 + 8); // ~40 pairs per block

struct Pair {
    char key[MAX_KEY_LEN + 1];
    int value;
    
    Pair() : value(0) {
        memset(key, 0, sizeof(key));
    }
    
    bool operator<(const Pair& other) const {
        int cmp = strcmp(key, other.key);
        if (cmp != 0) return cmp < 0;
        return value < other.value;
    }
    
    bool operator==(const Pair& other) const {
        return strcmp(key, other.key) == 0 && value == other.value;
    }
};

class BPlusTree {
private:
    const char* filename = "storage.dat";
    fstream file;
    
    struct Node {
        bool is_leaf;
        int count;
        Pair pairs[MAX_PAIRS];
        long children[MAX_PAIRS + 1];
        long next;
        
        Node() : is_leaf(true), count(0), next(-1) {
            memset(children, -1, sizeof(children));
        }
    };
    
    long root_pos;
    Node root;
    
    void open_file() {
        file.open(filename, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            file.open(filename, ios::out | ios::binary);
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);
            root_pos = 0;
            root = Node();
            write_node(root_pos, root);
        } else {
            file.seekg(0, ios::end);
            if (file.tellg() == 0) {
                root_pos = 0;
                root = Node();
                write_node(root_pos, root);
            } else {
                root_pos = 0;
                read_node(root_pos, root);
            }
        }
    }
    
    void read_node(long pos, Node& node) {
        file.seekg(pos);
        file.read((char*)&node, sizeof(Node));
    }
    
    void write_node(long pos, const Node& node) {
        if (pos == -1) {
            file.seekp(0, ios::end);
            pos = file.tellp();
        }
        file.seekp(pos);
        file.write((const char*)&node, sizeof(Node));
        file.flush();
    }
    
    long alloc_node() {
        file.seekp(0, ios::end);
        return file.tellp();
    }
    
    void insert_internal(long node_pos, Node& node, const Pair& pair, long child_pos = -1) {
        if (node.is_leaf) {
            int i = node.count - 1;
            while (i >= 0 && pair < node.pairs[i]) {
                node.pairs[i + 1] = node.pairs[i];
                i--;
            }
            node.pairs[i + 1] = pair;
            node.count++;
            write_node(node_pos, node);
            
            if (node.count >= MAX_PAIRS) {
                split_node(node_pos, node);
            }
        } else {
            int i = 0;
            while (i < node.count && !(pair < node.pairs[i])) {
                i++;
            }
            
            Node child;
            read_node(node.children[i], child);
            insert_internal(node.children[i], child, pair);
            
            read_node(node_pos, node);
        }
    }
    
    void split_node(long node_pos, Node& node) {
        Node new_node;
        new_node.is_leaf = node.is_leaf;
        
        int mid = node.count / 2;
        
        if (node.is_leaf) {
            new_node.count = node.count - mid;
            for (int i = 0; i < new_node.count; i++) {
                new_node.pairs[i] = node.pairs[mid + i];
            }
            node.count = mid;
            
            new_node.next = node.next;
            long new_pos = alloc_node();
            node.next = new_pos;
            
            write_node(new_pos, new_node);
            write_node(node_pos, node);
            
            if (node_pos == root_pos) {
                Node new_root;
                new_root.is_leaf = false;
                new_root.count = 1;
                new_root.pairs[0] = new_node.pairs[0];
                new_root.children[0] = root_pos;
                new_root.children[1] = new_pos;
                
                long old_root_pos = root_pos;
                root_pos = alloc_node();
                write_node(root_pos, new_root);
                root = new_root;
            } else {
                insert_to_parent(node_pos, new_node.pairs[0], new_pos);
            }
        } else {
            new_node.count = node.count - mid - 1;
            for (int i = 0; i < new_node.count; i++) {
                new_node.pairs[i] = node.pairs[mid + 1 + i];
                new_node.children[i] = node.children[mid + 1 + i];
            }
            new_node.children[new_node.count] = node.children[node.count];
            
            Pair up_key = node.pairs[mid];
            node.count = mid;
            
            long new_pos = alloc_node();
            write_node(new_pos, new_node);
            write_node(node_pos, node);
            
            if (node_pos == root_pos) {
                Node new_root;
                new_root.is_leaf = false;
                new_root.count = 1;
                new_root.pairs[0] = up_key;
                new_root.children[0] = root_pos;
                new_root.children[1] = new_pos;
                
                root_pos = alloc_node();
                write_node(root_pos, new_root);
                root = new_root;
            } else {
                insert_to_parent(node_pos, up_key, new_pos);
            }
        }
    }
    
    void insert_to_parent(long left_pos, const Pair& key, long right_pos) {
        vector<long> path;
        find_parent(root_pos, left_pos, path);
        
        if (!path.empty()) {
            long parent_pos = path.back();
            Node parent;
            read_node(parent_pos, parent);
            
            int i = parent.count - 1;
            while (i >= 0 && !(key < parent.pairs[i])) {
                parent.pairs[i + 1] = parent.pairs[i];
                parent.children[i + 2] = parent.children[i + 1];
                i--;
            }
            parent.pairs[i + 1] = key;
            parent.children[i + 2] = right_pos;
            parent.count++;
            
            write_node(parent_pos, parent);
            
            if (parent.count >= MAX_PAIRS) {
                split_node(parent_pos, parent);
            }
        }
    }
    
    bool find_parent(long node_pos, long target_pos, vector<long>& path) {
        if (node_pos == target_pos) {
            return true;
        }
        
        Node node;
        read_node(node_pos, node);
        
        if (node.is_leaf) {
            return false;
        }
        
        for (int i = 0; i <= node.count; i++) {
            if (node.children[i] != -1) {
                path.push_back(node_pos);
                if (find_parent(node.children[i], target_pos, path)) {
                    return true;
                }
                path.pop_back();
            }
        }
        return false;
    }
    
public:
    BPlusTree() {
        open_file();
    }
    
    ~BPlusTree() {
        if (file.is_open()) {
            file.close();
        }
    }
    
    void insert(const char* key, int value) {
        Pair pair;
        strncpy(pair.key, key, MAX_KEY_LEN);
        pair.value = value;
        
        read_node(root_pos, root);
        insert_internal(root_pos, root, pair);
        read_node(root_pos, root);
    }
    
    void remove(const char* key, int value) {
        Pair pair;
        strncpy(pair.key, key, MAX_KEY_LEN);
        pair.value = value;
        
        Node node;
        long pos = root_pos;
        read_node(pos, node);
        
        while (!node.is_leaf) {
            int i = 0;
            while (i < node.count && !(pair < node.pairs[i])) {
                i++;
            }
            pos = node.children[i];
            read_node(pos, node);
        }
        
        for (int i = 0; i < node.count; i++) {
            if (node.pairs[i] == pair) {
                for (int j = i; j < node.count - 1; j++) {
                    node.pairs[j] = node.pairs[j + 1];
                }
                node.count--;
                write_node(pos, node);
                break;
            }
        }
    }
    
    vector<int> find(const char* key) {
        vector<int> result;
        
        Node node;
        long pos = root_pos;
        read_node(pos, node);
        
        while (!node.is_leaf) {
            int i = 0;
            Pair temp;
            strncpy(temp.key, key, MAX_KEY_LEN);
            temp.value = 0;
            
            while (i < node.count && !(temp < node.pairs[i])) {
                i++;
            }
            pos = node.children[i];
            read_node(pos, node);
        }
        
        while (pos != -1) {
            for (int i = 0; i < node.count; i++) {
                if (strcmp(node.pairs[i].key, key) == 0) {
                    result.push_back(node.pairs[i].value);
                } else if (strcmp(node.pairs[i].key, key) > 0) {
                    return result;
                }
            }
            pos = node.next;
            if (pos != -1) {
                read_node(pos, node);
            }
        }
        
        return result;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    BPlusTree tree;
    
    int n;
    cin >> n;
    
    for (int i = 0; i < n; i++) {
        string cmd;
        cin >> cmd;
        
        if (cmd == "insert") {
            string index;
            int value;
            cin >> index >> value;
            tree.insert(index.c_str(), value);
        } else if (cmd == "delete") {
            string index;
            int value;
            cin >> index >> value;
            tree.remove(index.c_str(), value);
        } else if (cmd == "find") {
            string index;
            cin >> index;
            vector<int> results = tree.find(index.c_str());
            
            if (results.empty()) {
                cout << "null\n";
            } else {
                sort(results.begin(), results.end());
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
