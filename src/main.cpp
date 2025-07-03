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
        VarType& var_type = symbol->var_type;

        if (var_type.base_type.size()) {
            cout << node->type << ": ";

            cout << var_type.base_type << " ";
            {
                int diff = (int)var_type.dimensions.size() - (int)node->array_indices.size();
                assert(diff >= 0);

                int sz = var_type.dimensions.size();
                for (int i = sz-diff; i < sz; i++) {
                    cout << "[";
                    cout << var_type.dimensions[i];
                    cout << "]";
                }
            }

            cout << node->token.lexeme;

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

VarType addTypesDfs(SymbolTable& symbol_table, Node* node, VarType her) {
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
        VarType sin = {node->children[0]->grammar_name, {}};
        sin = addTypesDfs(symbol_table, node->children[2], sin);

        Symbol* symbol = symbol_table.lookup(node->children[1]->token.lexeme);
        symbol->var_type = sin;

        for (Node* node : node->children) {
            addTypesDfs(symbol_table, node, {"", {}});
        }
    } else if (node->grammar_name == "PARAMLIST") {
        if (!node->children.size()) {
            return {"", {}};
        }

        VarType sin = {node->children[0]->grammar_name, {}};

        Symbol* symbol = symbol_table.lookup(node->children[1]->token.lexeme);
        symbol->var_type = sin;

        for (Node* node : node->children) {
            addTypesDfs(symbol_table, node, {"", {}});
        }
    } else if (node->grammar_name == "NU") {
        if (!node->children.size()) {
            return her;
        }

        assert(node->children.size() >= 4);

        Token token = node->children[1]->token;
        VarType& sin = her;
        sin.dimensions.push_back(stoi(token.lexeme));
        sin = addTypesDfs(symbol_table, node->children[3], sin);
        return sin;
    } else {
        for (Node* node : node->children) {
            addTypesDfs(symbol_table, node, {"", {}});
        }
    }

    return {"", {}};
}

void addTypes(SymbolTable& symbol_table, Node* tree) {
    addTypesDfs(symbol_table, tree, {"", {}});
}

VarType checkTypesDfs(SymbolTable& symbol_table, ExprNode* node, bool& success) {
    if (node->type == "ident") {
        Symbol* symbol = symbol_table.lookup(node->token.lexeme);

        VarType type = symbol->var_type;
        type.dimensions.resize(type.dimensions.size() - node->array_indices.size());
        return type;
    } else if (node->type == "const") {
        if (node->token.type == INT_CONST)         return {"int", {}};
        else if (node->token.type == FLOAT_CONST)  return {"float", {}};
        else if (node->token.type == STRING_CONST) return {"string", {}};
        else                                       assert(false);
    } else {
        if (node->child1 && node->child2) {
            VarType type1 = checkTypesDfs(symbol_table, node->child1, success);
            VarType type2 = checkTypesDfs(symbol_table, node->child2, success);
            if (type1 != type2) {
                success = false;
                return {"", {}};
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

struct IntermData {
    string code;

    string last_attribution;
    string next;
    int arg_count;

    string t_in;
    string t_out;
};

string nextT() {
    static int i = 0;
    return "t" + to_string(i++);
}

string nextLabel() {
    static int i = 0;
    return "label" + to_string(i++);
}

void generateIntermediateCodeDfs(Node* node, IntermData* data) {
    bool is_defining_function =
        node->children.size()
     && (
            node->grammar_name == "FUNCDEF"
         || node->grammar_name == "FUNCLIST'"
        );

    bool is_parameter_decl =
        node->children.size()
     && (
            node->grammar_name == "PARAMLIST"
         || node->grammar_name == "PARAMLIST'"
         || node->grammar_name == "PARAMLIST''"
         || node->grammar_name == "PARAMLIST'''"
        );

    bool is_function_call =
        node->children.size()
     && (
            node->grammar_name == "FUNCCALL"
         || node->grammar_name == "PARAMLISTCALL"
         || node->grammar_name == "PARAMLISTCALL'"
         || (
                node->grammar_name == "ATRIBSTAT'"
             && node->children.size() >= 2
             && node->children[1]->children.size() >= 2
             && node->children[1]->children[1]->grammar_name == "PARAMLISTCALL"
            )
         || (
                node->grammar_name == "ATRIBSTAT''" && node->children[0]->grammar_name == "(")
        );

    bool is_attribution =
        node->children.size()
     && (
            node->grammar_name == "ATRIBSTAT"
         || (node->grammar_name == "STATELIST" && node->children[0]->grammar_name == "LVALUE")
         || (node->grammar_name == "STATELIST'" && node->children[0]->grammar_name == "ident")
        );

    bool is_print =
        node->children.size()
     && node->children[0]->grammar_name == "print";

    bool is_read =
        node->children.size()
     && node->children[0]->grammar_name == "read";

    bool is_return =
        node->children.size()
     && node->children[0]->grammar_name == "return";

    bool is_terminal = islower(node->grammar_name[0]);

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

    bool is_nc =
        node->grammar_name == "NC";

    bool arithm_forward =
        node->grammar_name == "EXPRESSION"
     || node->grammar_name == "NUMEXPRESSION"
     || node->grammar_name == "TERM"
     || node->grammar_name == "FACTOR"
     || node->grammar_name == "LVALUE"
     || in_arithm_atribstat1
     || in_arithm_atribstat2;

    bool in_arithm_node = one_oper || two_oper || is_nc || arithm_forward;

    bool is_if =
        node->children.size()
     && node->children[0]->grammar_name == "if";

    bool is_for =
        node->children.size()
     && node->children[0]->grammar_name == "for";

    bool is_break =
        node->children.size()
     && node->children[0]->grammar_name == "break";

    bool is_new =
        node->children.size()
     && (
            node->children[0]->grammar_name == "new"
         || node->grammar_name == "ALLOCEXPRESSION'"
         || node->grammar_name == "ND"
        );


    auto recurse = [&](Node* child, IntermData* child_data) {
        if (!child_data->next.size()) {
            child_data->next = data->next;
        }
        if (!child_data->last_attribution.size()) {
            child_data->last_attribution = data->last_attribution;
        }
        generateIntermediateCodeDfs(child, child_data);
    };

    if (is_defining_function) {
        // FUNCDEF -> def ident ( PARAMLIST ) { STATELIST }
        // FUNCLIST' -> def ident ( PARAMLIST ) { STATELIST } FUNCLIST'

        string& ident = node->children[1]->token.lexeme;

        Node* statelist = node->children[6];
        IntermData statelist_data{};
        recurse(statelist, &statelist_data);

        Node* paramlist = node->children[3];
        IntermData paramlist_data{};
        recurse(paramlist, &paramlist_data);

        data->code += "\n" + ident + ":\n"
                    + paramlist_data.code
                    + statelist_data.code;

        if (node->children.size() > 8) {
            IntermData funclist_data{};
            Node* funclist = node->children.back();
            recurse(funclist, &funclist_data);
            data->code += funclist_data.code;
        }
    } else if (is_parameter_decl) {
        // PARAMLIST -> string ident PARAMLIST'''
        // PARAMLIST -> float ident PARAMLIST''
        // PARAMLIST -> int ident PARAMLIST'
        // PARAMLIST' -> , PARAMLIST
        // PARAMLIST'' -> , PARAMLIST
        // PARAMLIST''' -> , PARAMLIST

        if (node->grammar_name == "PARAMLIST") {
            string ident = node->children[1]->token.lexeme;

            Node* paramlist1 = node->children[2];
            IntermData paramlist1_data{};
            paramlist1_data.arg_count = data->arg_count;

            recurse(paramlist1, &paramlist1_data);

            data->code += ident + " = arg" + to_string(data->arg_count) + "\n"
                        + paramlist1_data.code;
        } else {
            Node* paramlist = node->children[1];
            IntermData paramlist_data{};
            paramlist_data.arg_count = data->arg_count + 1;

            recurse(paramlist, &paramlist_data);

            data->code += paramlist_data.code;
        }
    } else if (is_return) {
        // RETURNSTAT -> return
        // STATELIST -> return ; STATELIST'
        // STATELIST' -> return ; STATELIST'
        
        data->code += "retval = " + data->last_attribution + "\n";
        data->code += "return\n";

        if (node->children.size() > 1) {
            Node* statelist = node->children[2];
            IntermData statelist_data{};

            recurse(statelist, &statelist_data);

            data->code += statelist_data.code;
        }
    } else if (is_new) {
        // ALLOCEXPRESSION -> new ALLOCEXPRESSION'
        // ATRIBSTAT' -> new ALLOCEXPRESSION'
        // ALLOCEXPRESSION' -> int [ NUMEXPRESSION ] ND
        // ALLOCEXPRESSION' -> float [ NUMEXPRESSION ] ND
        // ALLOCEXPRESSION' -> string [ NUMEXPRESSION ] ND
        // ND -> [ NUMEXPRESSION ] ND

        if (node->children[0]->grammar_name == "new") {
            Node* allocexpression1 = node->children[1];
            IntermData allocexpression1_data{};

            recurse(allocexpression1, &allocexpression1_data);

            data->t_out += "new " + allocexpression1_data.t_out + "\n";
        } else if (node->grammar_name != "ND") {
            Node* numexpression = node->children[2]; 
            Node* nd = node->children[4];

            IntermData numexpression_data{};
            IntermData nd_data{};

            recurse(numexpression, &numexpression_data);
            recurse(nd, &nd_data);

            string type = node->children[0]->grammar_name;

            data->t_out += type + "[" + numexpression_data.t_out + "]" + nd_data.t_out;
        } else {
            Node* numexpression = node->children[1]; 
            Node* nd = node->children[3];

            IntermData numexpression_data{};
            IntermData nd_data{};

            recurse(numexpression, &numexpression_data);
            recurse(nd, &nd_data);
            
            data->t_out += "[" + numexpression_data.t_out + "]" + nd_data.t_out;
        }
    } else if (is_print) {
        // PRINTSTAT -> print EXPRESSION
        // STATELIST -> print EXPRESSION ; STATELIST'
        // STATELIST' -> print EXPRESSION ; STATELIST'

        Node* expression = node->children[1];
        IntermData expression_data{};

        recurse(expression, &expression_data);

        data->code += expression_data.code
                    + "print " + expression_data.t_out + "\n";

        if (node->children.size() > 2) {
            Node* statelist = node->children[3];
            IntermData statelist_data{};

            recurse(statelist, &statelist_data);

            data->code += statelist_data.code;
        }
    } else if (is_read) {
        // READSTAT -> read LVALUE
        // STATELIST -> read LVALUE ; STATELIST'
        // STATELIST' -> read LVALUE ; STATELIST'

        Node* lvalue = node->children[1];
        IntermData lvalue_data{};

        recurse(lvalue, &lvalue_data);

        data->code += lvalue_data.code
                    + "read " + lvalue_data.t_out + "\n";

        if (node->children.size() > 2) {
            Node* statelist = node->children[3];
            IntermData statelist_data{};

            recurse(statelist, &statelist_data);

            data->code += statelist_data.code;
        }
    } else if (is_function_call) {
        // FUNCCALL -> ident ( PARAMLISTCALL )
        // PARAMLISTCALL -> ident PARAMLISTCALL'
        // PARAMLISTCALL' -> , PARAMLISTCALL
        // ATRIBSTAT' -> ident ATRIBSTAT''
        // ATRIBSTAT'' -> ( PARAMLISTCALL )

        if (node->grammar_name == "FUNCCALL") {
            string ident = node->children[0]->token.lexeme;

            Node* paramlistcall = node->children[2];
            IntermData paramlistcall_data{};

            recurse(paramlistcall, &paramlistcall_data);

            data->code += paramlistcall_data.code
                        + "call " + ident + "\n";

            data->t_out = "retval";
        } else if (node->grammar_name == "ATRIBSTAT'") {
            string ident = node->children[0]->token.lexeme;

            Node* atribstat2 = node->children[1];
            IntermData atribstat2_data{};

            recurse(atribstat2, &atribstat2_data);

            data->code += atribstat2_data.code
                        + "call " + ident + "\n";

            data->t_out = "retval";
        } else if (node->grammar_name == "ATRIBSTAT''") {
            Node* paramlistcall = node->children[1];
            IntermData paramlistcall_data{};

            recurse(paramlistcall, &paramlistcall_data);

            data->code += paramlistcall_data.code;
        } else if (node->grammar_name == "PARAMLISTCALL") {
            string ident = node->children[0]->token.lexeme;

            Node* paramlistcall1 = node->children[1];
            IntermData paramlistcall1_data{};
            paramlistcall1_data.arg_count = data->arg_count;

            recurse(paramlistcall1, &paramlistcall1_data);

            data->code += "arg" + to_string(data->arg_count) + " = " + ident + "\n"
                        + paramlistcall1_data.code;
        } else if (node->grammar_name == "PARAMLISTCALL'") {
            Node* paramlistcall = node->children[1];
            IntermData paramlistcall_data{};
            paramlistcall_data.arg_count = data->arg_count + 1;

            recurse(paramlistcall, &paramlistcall_data);

            data->code += paramlistcall_data.code;
        }
    } else if (is_attribution) {
        // ATRIBSTAT -> LVALUE = ATRIBSTAT'
        // STATELIST -> LVALUE = ATRIBSTAT' ; STATELIST'
        // STATELIST' -> ident NC = ATRIBSTAT' ; STATELIST'

        bool statelist1 = (node->grammar_name == "STATELIST'");

        string block_code;
        string lvalue_s;
        string last_t;

        Node* rest_node = 0;

        if (statelist1) {
            Node* ident = node->children[0];
            Node* nc = node->children[1];
            Node* atribstat1 = node->children[3];

            IntermData ident_data{};
            IntermData nc_data{};
            IntermData atribstat1_data{};

            nc_data.t_in = ident->token.lexeme;

            recurse(ident, &ident_data);
            recurse(nc, &nc_data);
            recurse(atribstat1, &atribstat1_data);

            block_code = atribstat1_data.code;
            lvalue_s = nc_data.t_out;
            last_t = atribstat1_data.t_out;

            rest_node = node->children[5];
        } else {
            Node* lvalue = node->children[0];
            Node* atribstat1 = node->children[2];

            IntermData lvalue_data{};
            IntermData atribstat1_data{};

            recurse(lvalue, &lvalue_data);
            recurse(atribstat1, &atribstat1_data);

            block_code = atribstat1_data.code;
            lvalue_s = lvalue_data.t_out;
            last_t = atribstat1_data.t_out;

            if (node->grammar_name == "STATELIST") {
                rest_node = node->children[4];
            }
        }

        data->code += block_code;
        data->code += lvalue_s + " = " + last_t + "\n";

        data->last_attribution = lvalue_s;

        if (rest_node) {
            IntermData child_data{};
            recurse(rest_node, &child_data);
            data->code += child_data.code;
        }
    } else if (is_terminal) {
        data->t_out = node->token.lexeme;
    } else if (in_arithm_node) {
        data->t_out = data->t_in;
        if (two_oper) {
            if (!node->children.size()) return;

            string symbol = node->children[0]->grammar_name;

            IntermData operand2_data{};
            recurse(node->children[1], &operand2_data);

            string new_t = nextT();
            data->code += new_t + " = " + data->t_in + " " + symbol + " " + operand2_data.t_out
                + "\n";
            data->t_out = new_t;

            if (node->children.size() >= 3) {
                IntermData rest_data{};
                rest_data.t_in = new_t;
                recurse(node->children[2], &rest_data);
                data->code += rest_data.code;
                data->t_out = rest_data.t_out;
            }
        } else if (is_nc) {
            if (!node->children.size()) {
                data->t_out = data->t_in;
                return;
            }

            // NC -> [ NUMEXPRESSION ] NC
            // NC -> ϵ

            Node* nc = node->children[3];
            IntermData nc_data{};

            Node* numexpression = node->children[1];
            IntermData numexpression_data{};

            recurse(numexpression, &numexpression_data);
            recurse(nc, &nc_data);

            data->t_out = data->t_in + "[" + numexpression_data.t_out + "]" + nc_data.t_out;
        } else {
            string last_t = data->t_in;
            for (Node* child : node->children) {
                IntermData child_data{};
                child_data.t_in = last_t;
                recurse(child, &child_data);
                last_t = child_data.t_out;
                data->code += child_data.code;
            }
            data->t_out = last_t;
        }
    } else if (is_if) {
        Node* expression = node->children[2];
        IntermData expression_data{};

        recurse(expression, &expression_data);

        Node* statement = node->children[4];
        IntermData statement_data{};

        Node* else_node = node->children[5];

        recurse(statement, &statement_data);

        if (else_node->children.size()) {
            string else_label = nextLabel();
            string end_label = nextLabel();

            IntermData else_data{};
            recurse(else_node, &else_data);

            data->code += "\n"
                        + expression_data.code
                        + "if False " + expression_data.t_out + " goto " + else_label + "\n"
                        + statement_data.code
                        + "goto " + end_label + "\n"
                        + else_label + ":\n"
                        + else_data.code
                        + end_label + ":\n"
                        + "\n";
        } else {
            string end_label = nextLabel();

            data->code += "\n"
                        + expression_data.code
                        + "if False " + expression_data.t_out + " goto " + end_label + "\n"
                        + statement_data.code
                        + end_label + ":\n"
                        + "\n";
        }

        if (node->children.size() > 6) {
            Node* statelist1 = node->children.back();
            IntermData statelist1_data;
            recurse(statelist1, &statelist1_data);
            data->code += statelist1_data.code;
        }

        //IFSTAT     -> if ( EXPRESSION ) STATEMENT IFSTAT'
        //STATELIST  -> if ( EXPRESSION ) STATEMENT IFSTAT' STATELIST'
        //STATELIST' -> if ( EXPRESSION ) STATEMENT IFSTAT' STATELIST'
    } else if (is_for) {
        // FORSTAT -> for ( ATRIBSTAT ; EXPRESSION ; ATRIBSTAT ) STATEMENT
        // STATELIST -> for ( ATRIBSTAT ; EXPRESSION ; ATRIBSTAT ) STATEMENT STATELIST'
        // STATELIST' -> for ( ATRIBSTAT ; EXPRESSION ; ATRIBSTAT ) STATEMENT STATELIST'

        Node* atribstat_init = node->children[2];
        Node* expression     = node->children[4];
        Node* atribstat_inc  = node->children[6];
        Node* statement      = node->children[8];

        IntermData atribstat_init_data{};
        IntermData expression_data{};
        IntermData atribstat_inc_data{};
        IntermData statement_data{};

        string start_label = nextLabel();
        string end_label   = nextLabel();

        recurse(atribstat_init, &atribstat_init_data);
        recurse(expression, &expression_data);
        recurse(atribstat_inc, &atribstat_inc_data);

        statement_data.next = end_label;
        recurse(statement, &statement_data);

        data->code += "\n"
                    + atribstat_init_data.code
                    + start_label + ":\n"
                    + expression_data.code
                    + "if False " + expression_data.t_out + " goto " + end_label + "\n"
                    + statement_data.code
                    + atribstat_inc_data.code
                    + "goto " + start_label + "\n"
                    + end_label + ":\n"
                    + "\n";

        if (node->children.size() > 9) {
            Node* statelist = node->children[9];
            IntermData statelist_data{};

            recurse(statelist, &statelist_data);

            data->code += statelist_data.code;
        }
    } else if (is_break) {
        // STATEMENT -> break ;
        // STATELIST -> break ; STATELIST'
        // STATELIST' -> break ; STATELIST'

        data->code += "goto " + data->next + "\n";

        if (node->children.size() > 2) {
            Node* statelist = node->children[2];
            IntermData statelist_data{};
            recurse(statelist, &statelist_data);

            data->code += statelist_data.code;
        }
    } else {
        for (Node* child : node->children) {
            IntermData child_data{};
            recurse(child, &child_data);
            data->code += child_data.code;
        }
    }
}

string generateIntermediateCode(Node* tree) {
    IntermData data{};
    generateIntermediateCodeDfs(tree, &data);
    return data.code;
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

    bool success = checkScope(tree);
    assert(success);

    cout << "Tree:\n";
    dfs_print(tree);

    cout << "\nExprTree:\n";
    for (ExprNode* expr_tree : expr_trees) {
        dfs_print(expr_tree, symbol_table);
        cout << "\n";
    }

    string intermediate_code = generateIntermediateCode(tree);
    cout << "Intermediate code:\n";
    cout << intermediate_code;

    return 0;
}
