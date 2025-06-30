#include "include/lexical_analyzer.hpp"
#include "include/symbol_table.hpp"
#include "include/syntax_analyzer.hpp"

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

void dfs_print(Node* node, int depth=0) {
    for (int i = 0; i < depth; i++) {
        cout << "-";
    }
    cout << node->grammar_name;
    if (node->token.type == IDENT) {
        cout << ": " << node->token.lexeme;
    }
    cout << "\n";

    for (Node* child : node->children) {
        dfs_print(child, depth+1);
    }
}

struct ExprNode {
    string type;

    // Operator type
    ExprNode* child1;
    ExprNode* child2;

    // Const & Ident type
    Token token;

    // Ident type
    vector<ExprNode*> array_indices;
};

ExprNode* createExprNodeOper(string type, ExprNode* child1, ExprNode* child2) {
    return new ExprNode{type, child1, child2};
}

ExprNode* createExprNodeConst(Token token) {
    return new ExprNode{"const", 0, 0, token};
}

ExprNode* createExprNodeIdent(Token token) {
    return new ExprNode{"ident", 0, 0, token, {}};
}

void dfs_print(ExprNode* node, SymbolTable& symbol_table, int depth=0) {
    for (int i = 0; i < depth; i++) {
        cout << "-";
    }
    if (node->type == "ident") {
        Symbol* symbol = symbol_table.lookup(node->token.lexeme);
        string var_type = symbol->var_type;

        if (var_type.size()) {
            cout << node->type << ": " << var_type << " " << node->token.lexeme
                << "\n";
        } else {
            cout << node->type << ": " << node->token.lexeme << "\n";
        }
    } else if (node->type == "const") {
        string s;
        switch (node->token.type) {
        case INT_CONST: {
            s = "int";
        } break;
        case FLOAT_CONST: {
            s = "float";
        } break;
        case STRING_CONST: {
            s = "string";
        } break;
        default: {
        } break;
        }
        cout << node->type << ": " << s << " " << node->token.lexeme << "\n";
    } else {
        cout << node->type << "\n";
        if (node->child1) {
            dfs_print(node->child1, symbol_table, depth+1);
        }
        if (node->child2) {
            dfs_print(node->child2, symbol_table, depth+1);
        }
    }
}

ExprNode* createExprTreesDfs(
    Node* node,
    vector<ExprNode*>& expr_trees,
    ExprNode* her
) {
    auto way1 = [&]() {
        ExprNode* sin;
        sin = createExprTreesDfs(node->children[0], expr_trees, 0);
        sin = createExprTreesDfs(node->children[1], expr_trees, sin);
        return sin;
    };

    auto way2 = [&]() {
        if (!node->children.size()) {
            return her;
        }

        string symbol = node->children[0]->grammar_name;

        ExprNode* sin;
        sin = createExprTreesDfs(node->children[0], expr_trees, 0);
        sin = createExprTreesDfs(node->children[1], expr_trees, sin);
        sin = createExprNodeOper(symbol, her, sin);
        sin = createExprTreesDfs(node->children[2], expr_trees, sin);
        return sin;
    };

    if (node->grammar_name == "EXPRESSION") {
        ExprNode* sin;
        sin = createExprTreesDfs(node->children[0], expr_trees, 0);
        expr_trees.push_back(sin);
    } else if (node->grammar_name == "NUMEXPRESSION") {
        return way1();
    } else if (node->grammar_name == "NT") {
        return way2();
    } else if (node->grammar_name == "TERM") {
        return way1();
    } else if (node->grammar_name == "NQ") {
        return way2();
    } else if (node->grammar_name == "UNARYEXPR") {
        ExprNode* sin;
        sin = createExprTreesDfs(node->children[0], expr_trees, 0);
        if (node->children[0]->grammar_name == "FACTOR") {
            return sin;
        } else {
            string symbol = node->children[0]->grammar_name;
            return createExprNodeOper(symbol, sin, 0);
        }
    } else if (node->grammar_name == "FACTOR") {
        string type = node->children[0]->grammar_name;
        if (type == "LVALUE") {
            return createExprTreesDfs(node->children[0], expr_trees, 0);
        } else if (type == "(") {
            ExprNode* sin;

            sin = createExprTreesDfs(node->children[1], expr_trees, 0);

            return sin;
        } else {
            return createExprNodeConst(node->children[0]->token);
        }
    } else if (node->grammar_name == "LVALUE") {
        ExprNode* sin;
        sin = createExprNodeIdent(node->children[0]->token);
        return createExprTreesDfs(node->children[1], expr_trees, sin);
    } else if (node->grammar_name == "NC") {
        ExprNode* sin;
        if (node->children.size()) {
            sin = createExprTreesDfs(node->children[1], expr_trees, 0);

            her->array_indices.push_back(sin);

            return createExprTreesDfs(node->children[3], expr_trees, sin);
        } else {
            return her;
        }
    } else if (node->grammar_name == "ATRIBSTAT'") {
        if (   node->children[0]->grammar_name == "int_constant"
            || node->children[0]->grammar_name == "float_constant"
            || node->children[0]->grammar_name == "string_constant"
            || node->children[0]->grammar_name == "null"
        ) {
            Token token = node->children[0]->token;
            ExprNode* sin = createExprNodeConst(token);
            sin = createExprTreesDfs(node->children[1], expr_trees, sin);
            sin = createExprTreesDfs(node->children[2], expr_trees, sin);
            expr_trees.push_back(sin);
        } else if (node->children[0]->grammar_name == "(") {
            ExprNode* sin;
            sin = createExprTreesDfs(node->children[1], expr_trees, 0);
            sin = createExprTreesDfs(node->children[3], expr_trees, 0);
            sin = createExprTreesDfs(node->children[4], expr_trees, 0);
            expr_trees.push_back(sin);
        } else if (node->children[0]->grammar_name == "+"
                || node->children[0]->grammar_name == "-") {
            ExprNode* sin;
            sin = createExprTreesDfs(node->children[1], expr_trees, 0);

            string oper = node->children[0]->grammar_name;
            sin = createExprNodeOper(oper, sin, 0);

            sin = createExprTreesDfs(node->children[3], expr_trees, 0);
            sin = createExprTreesDfs(node->children[4], expr_trees, 0);
            expr_trees.push_back(sin);
        } else if (node->children[0]->grammar_name == "ident") {
            ExprNode* sin;
            Token token = node->children[0]->token;
            sin = createExprNodeIdent(token);

            sin = createExprTreesDfs(node->children[1], expr_trees, sin);
            expr_trees.push_back(sin);
        } else {
            assert(false);
        }
    } else if (node->grammar_name == "ATRIBSTAT''") {
        if (!node->children.size()) {
            return her;
        }

        if (node->children[0]->grammar_name == "NC") {
            ExprNode* sin;
            sin = createExprTreesDfs(node->children[0], expr_trees, her);
            sin = createExprTreesDfs(node->children[1], expr_trees, her);
            sin = createExprTreesDfs(node->children[2], expr_trees, her);
            return sin;
        } else {
            assert(false);
        }
    } else {
        for (Node* child : node->children) {
            createExprTreesDfs(child, expr_trees, 0);
        }
    }
    
    return 0;
}

vector<ExprNode*> createExprTrees(Node* tree) {
    vector<ExprNode*> expr_trees;
    createExprTreesDfs(tree, expr_trees, 0);
    return expr_trees;
}

string addTypesDfs(SymbolTable& symbol_table, Node* node, string her) {
    assert(node);

    if (
        (
            node->grammar_name == "VARDECL"
         || node->grammar_name == "STATELIST"
         || node->grammar_name == "STATELIST'"
        ) && node->children.size() && (
            node->children[0]->grammar_name == "int"
         || node->children[0]->grammar_name == "float"
         || node->children[0]->grammar_name == "string"
        )
    ) {
        assert(node->children.size() >= 3);
        string sin = node->children[0]->grammar_name;
        sin = addTypesDfs(symbol_table, node->children[2], sin);

        Symbol* symbol = symbol_table.lookup(node->children[1]->token.lexeme);
        symbol->var_type = sin;
    } else if (node->grammar_name == "PARAMLIST") {
        if (!node->children.size()) {
            return "";
        }

        string sin = node->children[0]->grammar_name;

        Symbol* symbol = symbol_table.lookup(node->children[1]->token.lexeme);
        symbol->var_type = sin;

        for (Node* node : node->children) {
            addTypesDfs(symbol_table, node, "");
        }
    } else if (node->grammar_name == "NU") {
        if (!node->children.size()) {
            return her;
        }

        assert(node->children.size() >= 4);

        Token token = node->children[1]->token;
        string sin = her;
        sin += "[" + token.lexeme + "]";
        addTypesDfs(symbol_table, node->children[3], sin);
        return sin;
    } else {
        for (Node* node : node->children) {
            addTypesDfs(symbol_table, node, "");
        }
    }

    return "";
}

void addTypes(SymbolTable& symbol_table, Node* tree) {
    addTypesDfs(symbol_table, tree, "");
}

string checkTypesDfs(SymbolTable& symbol_table, ExprNode* node, bool& success) {
    if (node->type == "ident") {
        Symbol* symbol = symbol_table.lookup(node->token.lexeme);
        return symbol->var_type;
    } else if (node->type == "const") {
        if (node->token.type == INT_CONST)         return "int";
        else if (node->token.type == FLOAT_CONST)  return "float";
        else if (node->token.type == STRING_CONST) return "null";
        else                                       assert(false);
    } else {
        if (node->child1 && node->child2) {
            string type1 = checkTypesDfs(symbol_table, node->child1, success);
            string type2 = checkTypesDfs(symbol_table, node->child2, success);
            if (type1 != type2) {
                success = false;
                return "";
            }
            return type1;
        } else {
            return checkTypesDfs(symbol_table, node->child1, success);
        }
    }
}

bool checkTypes(SymbolTable& symbol_table, ExprNode* tree) {
    bool success = true;
    checkTypesDfs(symbol_table, tree, success);
    return success;
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

    // TODO: uncomment
    //lexer.print_tokens();
    //symbol_table.print();
    //cout << endl;


    SyntaxAnalyzer syntaxer(lexer.tokens);
    syntaxer.loadGrammar("./gramatica_LL1.txt");
    //syntaxer.printGrammar();
    syntaxer.computeAllFirst();
    syntaxer.computeFollow();
    syntaxer.buildParseTable();
    //syntaxer.printFirstSets();
    //syntaxer.printFollowSets();

    // TODO: uncomment
    //syntaxer.printParseTable();

    Node* tree = syntaxer.parse();
    if (!tree) return -1;

    //dfs_print(tree);

    vector<ExprNode*> expr_trees = createExprTrees(tree);
    addTypes(symbol_table, tree);

    for (ExprNode* expr_tree : expr_trees) {
        bool success = checkTypes(symbol_table, expr_tree);
        assert(success);
    }

    for (ExprNode* expr_tree : expr_trees) {
        dfs_print(expr_tree, symbol_table);
    }

    return 0;
}
