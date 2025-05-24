#pragma once

#include <string>
#include <vector>

using namespace std;

enum TokenType {
    // KEYWORDS
    // def int float string new print
    KW_DEF, KW_INT, KW_FLOAT, KW_STR, KW_NEW, KW_PRINT,
    // read return if else for break null
    KW_READ, KW_RETURN, KW_IF, KW_ELSE, KW_FOR, KW_BREAK, KW_NULL,

    // IDENTIFIERS
    IDENT,

    // CONSTANTS
    INT_CONST,
    FLOAT_CONST,
    STRING_CONST,

    // OPERATORS
    // + - * / %
    ARITH_OPER,
    // < > <= >= == !=
    RELAT_OPER,

    // PONCTUATIONS
    // ( )
    OPEN_PARENT, CLOSED_PARENT,
    // [ ]
    OPEN_SQRBRACK, CLOSED_SQRBRACK,
    // { }
    OPEN_BRACES, CLOSED_BRACES,
    // ,
    COMMA,
    // ;
    SEMICOLON,
    // =
    EQUAL
};

struct Token {
    TokenType type;
    string str_type;
    string lexeme;
    int line;
    int column;
};

struct Symbol {
    TokenType type;
    string str_type;
    string symbol;
    vector<pair<int, int>> occorrences;
};

const vector<Symbol> KEYWORDS_VECTOR = {
    {KW_DEF, "KW_DEF", "def", {}},
    {KW_INT, "KW_INT", "int", {}},
    {KW_FLOAT, "KW_FLOAT", "float", {}},
    {KW_STR, "KW_STR", "string", {}},
    {KW_NEW, "KW_NEW", "new", {}},
    {KW_PRINT, "KW_PRINT", "print", {}},
    {KW_READ, "KW_READ", "read", {}},
    {KW_RETURN, "KW_RETURN", "return", {}},
    {KW_IF, "KW_IF", "if", {}},
    {KW_ELSE, "KW_ELSE", "else", {}},
    {KW_FOR, "KW_FOR", "for", {}},
    {KW_BREAK, "KW_BREAK", "break", {}},
    {KW_NULL, "KW_NULL", "null", {}}
};
