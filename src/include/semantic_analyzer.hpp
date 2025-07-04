#pragma once

#include "utils.hpp"
#include "syntax_analyzer.hpp"
#include "symbol_table.hpp"

#include <string>
#include <vector>

using namespace std;

struct ExprNode {
    string type;
    Token token;

    // Operator type
    ExprNode* child1;
    ExprNode* child2;

    // Ident type
    vector<ExprNode*> array_indices;

    ~ExprNode() {
        if (child1) {
            delete child1;
        }
        if (child2) {
            delete child2;
        }
    }
};

class SemanticAnalyzer {
public:
    void dfs_print(Node* node, int depth=0);
    void dfs_print(ExprNode* node, SymbolTable& symbol_table, int depth=0);
    vector<ExprNode*> createExprTrees(Node* tree);
    void addTypes(SymbolTable& symbol_table, Node* tree);
    bool checkTypes(SymbolTable& symbol_table, ExprNode* tree);
    bool checkScope(Node* tree);

private:
    ExprNode* createExprNodeOper(Token token, ExprNode* child1, ExprNode* child2);
    ExprNode* createExprNodeConst(Token token);
    ExprNode* createExprNodeIdent(Token token);
    string varTypeToString(VarType& var_type, int ignore);
    ExprNode* createExprTreesDfs(
        Node* node,
        vector<ExprNode*>& expr_trees,
        ExprNode* her
    );
    VarType addTypesDfs(SymbolTable& symbol_table, Node* node, VarType her);
    VarType checkTypesDfs(SymbolTable& symbol_table, ExprNode* node, bool& success);
    bool checkScopeDfs(
        Node* node,
        vector<vector<string>>& scope,
        int loop_count
    );
};
