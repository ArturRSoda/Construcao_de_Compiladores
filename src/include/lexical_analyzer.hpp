#pragma once

#include "utils.hpp"
#include "symbol_table.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>

using namespace std;

class LexicalAnalyzer {
    private:
        SymbolTable* symbol_table;

        int line;
        int column;
        size_t lexeme_begin;
        size_t forward;
        string input;

        void errorHandler(string message);

        void reachForward();
        void advanceLexemeBegin();

        void initializeKeyWords();
        void addToken(TokenType type, string str_type, string lexeme);
        Symbol addToSymbolTable();

        bool getIdentifier();
        bool getNumber();
        bool getString();
        bool getOperator();
        bool getPonctuation();
        bool isWhiteSpace();


    public:
        LexicalAnalyzer(SymbolTable* table, string string_input);
        bool analyze();
        void print_tokens(string out_dir);
        vector<Token> tokens;
};
