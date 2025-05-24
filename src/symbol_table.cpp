#include "include/symbol_table.hpp"

using namespace std;

SymbolTable::SymbolTable() {
    id = 0;
}

bool SymbolTable::exists(string lexeme) {
    return table.find(lexeme) != table.end();
}

void SymbolTable::insertKeyWord(TokenType type, string str_type, string lexeme) {
    Symbol s = {
        type,
        str_type,
        lexeme,
        {}
    };
    table[lexeme] = s;
}

void SymbolTable::insert(TokenType type, string str_type, string lexeme, int line, int column) {
    if (!exists(lexeme)) {
        Symbol s = {
            type,
            str_type,
            lexeme,
            {{line, column}}
        };
        table[lexeme] = s;
    }
    else {
        table[lexeme].occorrences.push_back({line, column});
    }
}

Symbol* SymbolTable::lookup(string lexeme) {
    auto it = table.find(lexeme);
    if (it != table.end())
        return &it->second;
    else
        return nullptr;
}

void SymbolTable::print() {
    cout << "----- SYMBOL TABLE -----" << endl;
    cout << "lexeme - Token Type - occurrences (line, column)" << endl;
    for (auto x : table) {
        cout << x.first << " - " << x.second.str_type << " - ";
        for (auto y: x.second.occorrences) {
            cout << "(" << y.first << ", " << y.second << ") ";
        }
        cout << endl;
    }
}
