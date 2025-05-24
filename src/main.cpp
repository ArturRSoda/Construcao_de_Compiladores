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

int main() {
    string code = readInputCode("input_code.txt");
    //string code = "break";

    SymbolTable symbol_table;

    LexicalAnalyzer lexer(&symbol_table, code);
    lexer.analyze();

    cout << code << endl << endl;

    symbol_table.print();
    cout << endl;

    cout << "Lexeme - TokenType - Line - COlumn" << endl;
    for (Token t : lexer.tokens) {
        cout << t.lexeme << " - " << t.str_type << " - " << t.line << " - " << t.column << endl;
    }

    return 0;
}
