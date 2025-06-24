#include "include/utils.hpp"

using namespace std;

string trim(string s) {
    s.erase(remove(s.begin(), s.end(), ' '), s.end());
    return s;
}

vector<string> split(string line) {
    vector<string> tokens;
    istringstream iss(line);
    string token;
    while (iss >> token)
        tokens.push_back(token);
    return tokens;
}

string getTerminalName(Token token) {
    switch (token.type) {
        case KW_DEF: return "def";
        case KW_INT: return "int";
        case KW_FLOAT: return "float";
        case KW_STR: return "string";
        case KW_NEW: return "new";
        case KW_PRINT: return "print";
        case KW_READ: return "read";
        case KW_RETURN: return "return";
        case KW_IF: return "if";
        case KW_ELSE: return "else";
        case KW_FOR: return "for";
        case KW_BREAK: return "break";
        case KW_NULL: return "null";

        case IDENT: return "ident";
        case INT_CONST: return "int_constant";
        case FLOAT_CONST: return "float_constant";
        case STRING_CONST: return "string_constant";

        case ARITH_OPER: return token.lexeme; // Se quiser literal
        case RELAT_OPER: return token.lexeme;

        case OPEN_PARENT: return "(";
        case CLOSED_PARENT: return ")";
        case OPEN_SQRBRACK: return "[";
        case CLOSED_SQRBRACK: return "]";
        case OPEN_BRACES: return "{";
        case CLOSED_BRACES: return "}";
        case COMMA: return ",";
        case SEMICOLON: return ";";
        case EQUAL: return "=";

        case END_OF_FILE: return "$";

        default: return token.lexeme;
    }
}

