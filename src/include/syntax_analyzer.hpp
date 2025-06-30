#pragma once

#include "utils.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <algorithm>

using namespace std;

struct Node {
    string grammar_name;
    Token token;

    Node* parent;
    vector<Node*> children;
};

class SyntaxAnalyzer {
    private:
        map<string, vector<vector<string>>> grammar;
        map<string, set<string>> firstSet;
        map<string, set<string>> followSet;
        map<string, map<string, vector<string>>> parseTable;

        set<string> nonTerminals;
        set<string> terminals;
        set<string> possibleSymbols;
        string startSymbol;

        vector<Token> tokens;
        int currentTokenIndex;
        Token getCurrentToken();
        void advanceToken();

    public:
        SyntaxAnalyzer(vector<Token> t);
        void printGrammar();
        void printFirstSets();
        void printFollowSets();
        void printParseTable();

        void loadGrammar(string filename);
        set<string> computeFirst(string symbol);
        void computeAllFirst();
        void computeFollow();
        void buildParseTable();
        Node* parse();
};
