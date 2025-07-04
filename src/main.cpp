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

    cout << "Success in lexical analysis!" << endl;
    lexer.print_tokens("outputs/tokens_vec.txt");
    cout << endl;
    symbol_table.print("outputs/symbol_table.txt");
    cout << endl;

    SyntaxAnalyzer syntaxer(lexer.tokens);
    syntaxer.loadGrammar("./gramatica_LL1.txt");
    //syntaxer.printGrammar();
    syntaxer.computeAllFirst();
    syntaxer.computeFollow();
    syntaxer.buildParseTable();
    //syntaxer.printFirstSets();
    //syntaxer.printFollowSets();
    syntaxer.printParseTable("outputs/parse_table.txt");
    cout << endl;

    Node* tree = syntaxer.parse();
    if (!tree) return -1;

    cout << "\n";

    SemanticAnalyzer sem;

    vector<ExprNode*> expr_trees = sem.createExprTrees(tree);
    sem.addTypes(symbol_table, tree);
    cout << "Expression trees created successfully!" << endl;
    cout << endl;

    for (ExprNode* expr_tree : expr_trees) {
        bool success = sem.checkTypes(symbol_table, expr_tree);
        if (!success) {
            cerr << "ERROR WHEN CHECKING TYPES!!!\n";
            return EXIT_FAILURE;
        }
    }
    cout << "Arithmetic expressions are valid! (type checking)" << endl;
    cout << endl;


    bool success = sem.checkScope(tree);
    if (!success) {
        cerr << "ERROR WHEN CHECKING SCOPE!!!\n";
        return EXIT_FAILURE;
    }
    cout << "Declarations of variables by scope are valid! (including breaks in for loops)" << endl;
    cout << endl;

    string tree_out_dir = "outputs/derivation_tree.txt";
    ofstream tree_out_file;
    tree_out_file.open(tree_out_dir);
    tree_out_file << "===================\n";
    tree_out_file << "Tree\n";
    tree_out_file << "===================\n";
    sem.dfs_print(tree, 0, tree_out_file);
    tree_out_file.close();
    cout << "Derivation Tree was successfully saved on " << tree_out_dir << endl << endl;


    string expr_tree_out_dir = "outputs/expr_tree.txt";
    ofstream expr_tree_file;
    expr_tree_file.open(expr_tree_out_dir);
    expr_tree_file << "\n===================\n";
    expr_tree_file << "ExprTree\n";
    expr_tree_file << "===================\n";
    for (ExprNode* expr_tree : expr_trees) {
        sem.dfs_print(expr_tree, symbol_table, 0, expr_tree_file);
        expr_tree_file << "\n";
    }
    expr_tree_file.close();
    cout << "Expression Tree was successfully saved on " << tree_out_dir << endl << endl;

    IntermediateCodeGen interm;
    string intermediate_code = interm.generateIntermediateCode(tree);
    cout << "Intermediate code generated successfully!" << endl << endl;

    string intermediate_code_out_dir = "outputs/intermediate_code.txt";
    ofstream intermediate_code_out_file;
    intermediate_code_out_file.open(intermediate_code_out_dir);
    intermediate_code_out_file << "===================\n";
    intermediate_code_out_file << "Intermediate code\n";
    intermediate_code_out_file << "===================\n";
    intermediate_code_out_file << intermediate_code;
    cout << "Intermediate code was successfully saved on " << intermediate_code_out_dir << endl;
    intermediate_code_out_file.close();

    return 0;
}
