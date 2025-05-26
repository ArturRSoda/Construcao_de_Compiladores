#include "include/lexical_analyzer.hpp"
#include "include/symbol_table.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

string readInputCode(string input_file) {
    ifstream file(input_file);

    if (!file.is_open()) {
        cerr << "Error: Could not open file '" << input_file << "'\n";
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();

    file.close();
    return buffer.str(); // return file contents as string
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Right Usage: " << argv[0] << " <input_file>\n";
        return -1;
    }
    string code = readInputCode(argv[1]); 
    if (code.empty()) {
        cerr << "Error: No code to analyze.\n";
        return -1;
    }

    SymbolTable symbol_table;

    cout << code << endl << endl;
    LexicalAnalyzer lexer(&symbol_table, code);
    if (!lexer.analyze()) {
        return -1;
    }


    symbol_table.print();
    cout << endl;

    //lexer.print_tokens();
    return 0;
}
