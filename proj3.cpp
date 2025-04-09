#include <iostream>
#include <fstream>
#include <map>

using namespace std;

enum TokenType {ID, LAREN, RPAREN, SEMICOLON, ASSIGNOP, NUM_INT, END};

struct Block{
    int address;
    int size;
    int refCount;
    Block(int addr, int sz, int rc = 0) : address(addr), size(sz), refCount(rc){};

};

vector<Block> freeList;
map<string, Block> variables;
string input;
size_t pos = 0;

struct Token {
    TokenType type;
    string value;
};

int main(){
    int heapSize;
    string filename;

    cout << "Please enter the initial freelist (heap) size: ";
    cin >>heapSize;
    cin.ignore();
    cout <<"Please enter the name of an input file: ";
    getline(cin, filename);

    ifstream file(filename);
    if (!file.is_open()){
        cerr << "Could not open file: "<<filename<<endl;
        return 1;
    }
    string line;
    while (getline(file, line)) input += line + " ";
    file.close();

    freeList.push_back(Block(0,heapSize, 0));
    //lookahead = getNextToken();
    //prog();

    return 0;
}

