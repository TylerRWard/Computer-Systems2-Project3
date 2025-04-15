#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <string>

using namespace std;

struct Block {
    int start_address;
    int size;
    int ref_count;
};

struct Variable {
    string name;
    Block block;
};

class MemoryManager {
    vector<Block> free_list;
    map<string, Variable> variables;
public:
    MemoryManager(int heap_size) {
        free_list.push_back({0, heap_size, 0});
    }

    void alloc(const string& var_name, int size) {
        // Free existing variable if it exists
        if (variables.count(var_name)) {
            free(var_name);
        }
     
        for (auto it = free_list.begin(); it != free_list.end(); ++it) {
            if (it->size >= size) {
                // Create new block for variable
                Block new_block = {it->start_address, size, 1};
                variables[var_name] = {var_name, new_block};
                // Update free list
                it->start_address += size;
                it->size -= size;
                if (it->size == 0) {
                    free_list.erase(it);
                }
                // Always sort free list to maintain invariant
                sort(free_list.begin(), free_list.end(),
                     [](const Block& a, const Block& b) { return a.start_address < b.start_address; });
                return;
            }
        }
    }

    void copy(const string& var1, const string& var2) {
        if (var1 == var2) {
            return; 
        }
        if (variables.count(var2)) {
            if (variables.count(var1)) {
                free(var1);
            }
            variables[var1] = variables[var2]; 
            variables[var1].block.ref_count++; 
        }
    }

    void free(const string& var_name) {
        if (variables.count(var_name)) {
            Variable& var = variables[var_name];
            if (var.block.ref_count > 0) {
                var.block.ref_count--;
                if (var.block.ref_count == 0) {
                    // Return block to free list
                    free_list.push_back({var.block.start_address, var.block.size, 0});
                    sort(free_list.begin(), free_list.end(),
                         [](const Block& a, const Block& b) { return a.start_address < b.start_address; });
                }
            }
            variables.erase(var_name);
        }
    }

    void compress() {
        if (free_list.empty()) return;
        sort(free_list.begin(), free_list.end(),
             [](const Block& a, const Block& b) { return a.start_address < b.start_address; });
        for (size_t i = 0; i < free_list.size() - 1;) {
            if (free_list[i].start_address + free_list[i].size == free_list[i + 1].start_address) {
                free_list[i].size += free_list[i + 1].size;
                free_list.erase(free_list.begin() + i + 1);
            } else {
                ++i;
            }
        }
    }

    void dump() {
        cout << "Variables:\n";
        for (const auto& [name, var] : variables) {
            cout << name << ":" << var.block.start_address << "(" << var.block.size
                 << ") [" << var.block.ref_count << "]\n";
        }
        cout << "Free List:\n";
        if (free_list.empty()) {
            cout << "Empty\n";
        } else {
            for (size_t i = 0; i < free_list.size(); ++i) {
                cout << free_list[i].start_address << "(" << free_list[i].size << ") [0]";
                if (i < free_list.size() - 1) {
                    cout << ", ";
                }
            }
            cout << "\n";
        }
        cout << "============================================================\n";
    }
};
//Grammar
enum TokenType { ID, LPAREN, RPAREN, SEMICOLON, ASSIGNOP, NUM_INT, END };

struct Token {
    TokenType type;
    string value;
};

class Lexer {
    string input;
    size_t pos;
public:
    Lexer(const string& src) : input(src), pos(0) {}
    Token getNextToken() {
        while (pos < input.size() && isspace(input[pos])) pos++;
        if (pos >= input.size()) return {END, ""};
        
        char c = input[pos];
        if (isalpha(c)) {
            string id;
            while (pos < input.size() && isalnum(input[pos])) id += input[pos++];
            return {ID, id};
        }
        if (isdigit(c)) {
            string num;
            while (pos < input.size() && isdigit(input[pos])) num += input[pos++];
            return {NUM_INT, num};
        }
        switch (c) {
            case '(': pos++; return {LPAREN, "("};
            case ')': pos++; return {RPAREN, ")"};
            case ';': pos++; return {SEMICOLON, ";"};
            case '=': pos++; return {ASSIGNOP, "="};
            default: pos++; return {END, ""}; 
        }
    }
};

class Parser {
    Lexer lexer;
    Token lookahead;
    MemoryManager& mem;
public:
    Parser(const string& input, MemoryManager& m) : lexer(input), mem(m) {
        lookahead = lexer.getNextToken();
    }
    
    void match(TokenType type) {
        if (lookahead.type == type) lookahead = lexer.getNextToken();
        else throw runtime_error("Syntax error");
    }
    
    void prog() { slist(); }
    
    void slist() {
        if (lookahead.type != END) {
            stmt();
            match(SEMICOLON);
            slist();
        }
    }
    
    void stmt() {
        if (lookahead.type != ID) throw runtime_error("Expected ID");
        string id = lookahead.value;
        match(ID);
        
        if (lookahead.type == LPAREN) {
            match(LPAREN);
            if (lookahead.type == ID) {
                string var = lookahead.value;
                match(ID);
                match(RPAREN);
                if (id == "free") mem.free(var); 
            } else {
                match(RPAREN);
                if (id == "dump") mem.dump();
                else if (id == "compress") mem.compress();
            }
        } else if (lookahead.type == ASSIGNOP) {
            match(ASSIGNOP);
            rhs(id);
        }
    }
    
    void rhs(const string& var_name) {
        if (lookahead.type != ID) throw runtime_error("Expected ID");
        string id = lookahead.value;
        match(ID);
        
        if (lookahead.type == LPAREN) {
            match(LPAREN);
            if (lookahead.type != NUM_INT) throw runtime_error("Expected NUM_INT");
            int size = stoi(lookahead.value);
            match(NUM_INT);
            match(RPAREN);
            mem.alloc(var_name, size); 
        } else {
            mem.copy(var_name, id);
        }
    }
};

int main() {
    int heap_size;
    string filename, input;
    cout << "Please enter the initial freelist (heap) size: ";
    cin >> heap_size;
    cin.ignore();
    cout << "Please enter the name of an input file: ";
    getline(cin, filename);
    
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open file: " << filename << endl;
        return 1;
    }
    string line;
    while (getline(file, line)) input += line + " ";
    file.close();
    
    MemoryManager mem(heap_size);
    Parser parser(input, mem);
    parser.prog();
    
    return 0;
}