#include "include/lexical_analyzer.hpp"
#include "include/symbol_table.hpp"
#include "include/syntax_analyzer.hpp"
#include "include/semantic_analyzer.hpp"
#include "include/intermediate_code_gen.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include <assert.h>

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
    LexicalAnalyzer lexer(&symbol_table, code);
    if (!lexer.analyze()) return -1;
    //cout << code << endl << endl;

    lexer.print_tokens();
    symbol_table.print();
    cout << endl;


    SyntaxAnalyzer syntaxer(lexer.tokens);
    syntaxer.loadGrammar("./gramatica_LL1.txt");
    //syntaxer.printGrammar();
    syntaxer.computeAllFirst();
    syntaxer.computeFollow();
    syntaxer.buildParseTable();
    //syntaxer.printFirstSets();
    //syntaxer.printFollowSets();

    syntaxer.printParseTable();

    Node* tree = syntaxer.parse();
    if (!tree) return -1;

    cout << "\n";

    SemanticAnalyzer sem;

    vector<ExprNode*> expr_trees = sem.createExprTrees(tree);
    sem.addTypes(symbol_table, tree);

    for (ExprNode* expr_tree : expr_trees) {
        bool success = sem.checkTypes(symbol_table, expr_tree);
        if (!success) {
            cerr << "ERROR WHEN CHECKING TYPES!!!\n";
            return EXIT_FAILURE;
        }
    }

    bool success = sem.checkScope(tree);
    if (!success) {
        cerr << "ERROR WHEN CHECKING SCOPE!!!\n";
        return EXIT_FAILURE;
    }

    cout << "===================\n";
    cout << "Tree\n";
    cout << "===================\n";
    sem.dfs_print(tree);

    cout << "\n===================\n";
    cout << "ExprTree\n";
    cout << "===================\n";
    for (ExprNode* expr_tree : expr_trees) {
        sem.dfs_print(expr_tree, symbol_table);
        cout << "\n";
    }

    IntermediateCodeGen interm;
    string intermediate_code = interm.generateIntermediateCode(tree);
    cout << "===================\n";
    cout << "Intermediate code\n";
    cout << "===================\n";
    cout << intermediate_code;

    return 0;
}
