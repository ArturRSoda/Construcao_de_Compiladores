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
    Token token;

    // Operator type
    ExprNode* child1;
    ExprNode* child2;

    // Ident type
    vector<ExprNode*> array_indices;
};

ExprNode* createExprNodeOper(Token token, ExprNode* child1, ExprNode* child2) {
    return new ExprNode{"oper", token, child1, child2};
}

ExprNode* createExprNodeConst(Token token) {
    return new ExprNode{"const", token};
}

ExprNode* createExprNodeIdent(Token token) {
    return new ExprNode{"ident", token};
}

void dfs_print(ExprNode* node, SymbolTable& symbol_table, int depth=0) {
    for (int i = 0; i < depth; i++) {
        cout << "-";
    }
    if (node->type == "ident") {
        Symbol* symbol = symbol_table.lookup(node->token.lexeme);
        string var_type = symbol->var_type;

        if (var_type.size()) {
            cout << node->type << ": " << var_type << " " << node->token.lexeme;

            for (ExprNode* index_node : node->array_indices) {
                cout << "[";
                cout << index_node->token.lexeme;
                cout << "]";
            }

            cout << "\n";
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
        cout << node->token.lexeme << "\n";
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
    bool in_arithm_atribstat1 =
        node->grammar_name == "ATRIBSTAT'"
     && node->children[0]->grammar_name != "new"
     && node->children[1]->grammar_name != "PARAMLISTCALL";

    bool in_arithm_atribstat2 =
        node->grammar_name == "ATRIBSTAT''"
     && (
            !node->children.size()
         || node->children[1]->grammar_name != "PARAMLISTCALL"
        );

    bool one_oper =
        node->grammar_name == "UNARYEXPR";

    bool two_oper =
        node->grammar_name == "NT"
     || node->grammar_name == "NQ"
     || node->grammar_name == "EXPRESSION'";

    bool nc =
        node->grammar_name == "NC";

    bool forward =
        node->grammar_name == "EXPRESSION"
     || node->grammar_name == "NUMEXPRESSION"
     || node->grammar_name == "TERM"
     || node->grammar_name == "FACTOR"
     || node->grammar_name == "LVALUE"
     || in_arithm_atribstat1
     || in_arithm_atribstat2;

    bool do_push =
        node->grammar_name == "EXPRESSION"
     || in_arithm_atribstat1;

    assert(do_push ? forward : true);

    bool in_arithm_node = one_oper || two_oper || nc || forward;

    if (in_arithm_node && !forward) {
        if (one_oper) {
            if (node->children.size() == 1) {
                return createExprTreesDfs(node->children[0], expr_trees, her);
            } else {
                ExprNode* sin = createExprTreesDfs(
                    node->children[1], expr_trees, 0);

                string symbol = node->children[0]->grammar_name;

                return createExprNodeOper(node->children[0]->token, sin, 0);
            }
        } else if (two_oper) {
            if (!node->children.size()) return her;

            string symbol = node->children[0]->grammar_name;

            ExprNode* sin = createExprTreesDfs(
                node->children[1], expr_trees, 0);

            sin = createExprNodeOper(node->children[0]->token, her, sin);

            if (node->children.size() >= 3) {
                sin = createExprTreesDfs(node->children[2], expr_trees, sin);
            }

            return sin;
        } else if (nc) {
            if (node->children.size()) {
                ExprNode* sin = createExprTreesDfs(
                    node->children[1], expr_trees, 0);

                her->array_indices.push_back(sin);

                sin = her;

                return createExprTreesDfs(node->children[3], expr_trees, sin);
            } else {
                return her;
            }
        } else {
            assert(false);
        }
    } else {
        bool is_const =
            node->grammar_name == "int_constant"
         || node->grammar_name == "float_constant"
         || node->grammar_name == "string_constant"
         || node->grammar_name == "null";

        bool is_ident =
            node->grammar_name == "ident";

        if (is_const) {
            return createExprNodeConst(node->token);
        } else if (is_ident) {
            return createExprNodeIdent(node->token);
        }

        ExprNode* sin = her;
        for (Node* child : node->children) {
            sin = createExprTreesDfs(child, expr_trees, sin);
        }

        if (
            node->grammar_name == "ATRIBSTAT'"
         || node->grammar_name == "ATRIBSTAT''"
        ) {
            return 0;
        } else if (in_arithm_node && do_push) {
            expr_trees.push_back(sin);
            return 0;
        } else {
            return sin;
        }
    }
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

        for (Node* node : node->children) {
            addTypesDfs(symbol_table, node, "");
        }
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
        else if (node->token.type == STRING_CONST) return "string";
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

bool checkScopeDfs(
    Node* node,
    vector<vector<string>>& scope,
    int loop_count
) {
    for (Node* child : node->children) {
        /* Creating new scope */ {
            bool may_create_scope = 
                   node->grammar_name == "FUNCDEF"
                || node->grammar_name == "FUNCLIST'"
                || node->grammar_name == "STATEMENT"
                || node->grammar_name == "STATELIST"
                || node->grammar_name == "STATELIST'";

            if (may_create_scope) {
                bool func_push_back = 
                    (
                        node->grammar_name == "FUNCDEF"
                     || node->grammar_name == "FUNCLIST'"
                    ) && child->grammar_name == "(";

                bool stat_push_back =
                    (
                        node->grammar_name == "STATEMENT"
                     || node->grammar_name == "STATELIST"
                     || node->grammar_name == "STATELIST'"
                    ) && child->grammar_name == "{";

                bool do_push_back = func_push_back || stat_push_back;
                bool do_pop_back = child->grammar_name == "}";

                if (do_push_back) {
                    scope.push_back({});
                } else if (do_pop_back) {
                    scope.pop_back();
                }
            }
        }

        /* Declaring new ident */ {
            bool declaring_new_ident = 
                (
                    node->grammar_name == "FUNCDEF"
                 || node->grammar_name == "PARAMLIST"
                 || node->grammar_name == "VARDECL"
                 || node->grammar_name == "PARAMLISTCALL"
                 || node->grammar_name == "STATELIST"
                 || node->grammar_name == "FUNCLIST'"
                 || node->grammar_name == "STATELIST'"
                )
                && node->children[0]->grammar_name != "ident"
                && child->grammar_name == "ident";

            if (declaring_new_ident) {
                string ident = child->token.lexeme;

                vector<string>& last_scope = scope.back();

                bool found_in_scope =
                    find(
                        last_scope.begin(), last_scope.end(),
                        ident
                    )
                    != last_scope.end();

                if (found_in_scope) {
                    return false;
                }

                last_scope.push_back(ident);
            }
        }

        /* Using ident */ {
            bool using_ident = (
                     node->grammar_name == "FUNCCALL"
                  || node->grammar_name == "PARAMLISTCALL"
                  || node->grammar_name == "LVALUE"
                  || node->grammar_name == "ATRIBSTAT'"
                  || node->grammar_name == "STATELIST'"
                )
                && child->grammar_name == "ident";

            if (using_ident) {
                string ident = child->token.lexeme;

                bool found = false;
                for (vector<string>& specific_scope : scope) {
                    for (string& decl_ident : specific_scope) {
                        if (decl_ident == ident) found = true;
                    }
                }

                if (!found) return false;
            }
        }

        /* Check break */ {
            if (child->grammar_name == "break") {
                if (loop_count == 0) return false;
            }
        }

        bool in_for_body = false;

        /* Entering new for */ {
            bool entering_new_for = (
                    node->grammar_name == "FORSTAT"
                 || node->grammar_name == "STATELIST"
                 || node->grammar_name == "STATELIST'"
                )
                && node->children.size()
                && node->children[0]->grammar_name == "for";

            in_for_body = entering_new_for && child->grammar_name == "STATEMENT";
        }

        if (in_for_body) loop_count++;

        if (!checkScopeDfs(child, scope, loop_count)) {
            return false;
        }

        if (in_for_body) loop_count--;
    }

    return true;
}

bool checkScope(Node* tree) {
    vector<vector<string>> scope{vector<string>()};
    return checkScopeDfs(tree, scope, 0);
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

    //for (ExprNode* expr_tree : expr_trees) {
    //    bool success = checkTypes(symbol_table, expr_tree);
    //    assert(success);
    //}

    //bool success = checkScope(tree);
    //assert(success);

    cout << "Tree:\n";
    dfs_print(tree);

    cout << "\nExprTree:\n";
    for (ExprNode* expr_tree : expr_trees) {
        dfs_print(expr_tree, symbol_table);
        cout << "\n";
    }

    return 0;
}
