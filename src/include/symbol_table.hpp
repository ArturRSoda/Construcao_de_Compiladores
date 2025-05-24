#pragma once

#include "utils.hpp"

#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

class SymbolTable {
    private:
        int id;
        unordered_map<string, Symbol> table;

    public:
        SymbolTable();
        bool exists(string lexeme);
        void insertKeyWord(TokenType type, string str_type, string symbol);
        void insert(TokenType type, string str_type, string symbol, int line, int column);
        Symbol* lookup(string lexeme);

        void print();

};
