#include "include/lexical_analyzer.hpp"
#include "include/utils.hpp"
#include <cctype>
#include <string>

// Construtor analizador lexico
LexicalAnalyzer::LexicalAnalyzer(SymbolTable* table, string string_input) {
    symbol_table = table;
    input = string_input;
    lexeme_begin = 0;     // Aponta para o inicio de um lexema
    line = 1;             // Contador de linhas
    column = 1;           // Contador de colunas
    tokens = {};          // Lista de tokens identificados

    initializeKeyWords(); // Insere palavras-chave na tabela de simbolos
}


// Exibe mensagens de erro lexico com detalhes de localizacao
void LexicalAnalyzer::errorHandler(string message) {
    string spc1(message.length(), ' ');
    string spc2(to_string(line).length(), ' ');
    string spc3(to_string(column).length(), ' ');

    string s = input.substr(lexeme_begin, 10);

    cerr << "[Lexical Error]: " << message << "; Line " << line << ", Column " << column << ": \"..." << s << "...\"" << endl;
    cerr << "                 " << spc1    << "       " << spc2 << "         " << spc3   << "      ^" << endl;
}

// Insere palavras-chave pre-definidas na tabela de simbolos
void LexicalAnalyzer::initializeKeyWords() {
    for (Symbol s : KEYWORDS_VECTOR) {
        symbol_table->insertKeyWord(s.type, s.str_type, s.symbol);
    }

}


// Atualiza lexeme_begin para a posição atual do forward (apos reconhecer um token)
void LexicalAnalyzer::reachForward() {
    while (lexeme_begin < forward) {
        advanceLexemeBegin();
    }
}


// Avança o ponteiro lexeme_begin e atualiza linha/coluna
void LexicalAnalyzer::advanceLexemeBegin() {
    char c = input[lexeme_begin];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    lexeme_begin++;
}


// Adiciona um identificador a tabela de simbolos e retorna seu simbolo
Symbol LexicalAnalyzer::addToSymbolTable() {
    string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);

    symbol_table->insert(IDENT, "IDENT", lexeme, line, column);
    Symbol* s = symbol_table->lookup(lexeme);

    return *s;
}


// Cria um token e o adiciona no vetor de tokens
void LexicalAnalyzer::addToken(TokenType type, string str_type, string lexeme) {
    Token t = {
        type,
        str_type,
        lexeme,
        line,
        column
    };
    tokens.push_back(t);
}


// Automato para reconhecer identificadores (letras seguido de letras/dígitos)
bool LexicalAnalyzer::getIdentifier() {
    int state = 0;
    forward = lexeme_begin;
    char c;

    while (true) {
        c = input[forward];

        switch (state) {
            case 0: {
                if (isalpha(c)) { state = 1; forward++; }
                else { return false; }
                break;
            }

            case 1: {
                if (isalnum(c)) { state = 1; forward++; }
                else { state = 2; }
                break;
            }

            case 2: {
                Symbol s = addToSymbolTable();
                addToken(s.type, s.str_type, s.symbol);
                return true;
            }
        }
    }
}


// Automato para reconhecer numeros (inteiros ou floats)
bool LexicalAnalyzer::getNumber() {
    int state = 0;
    forward = lexeme_begin;
    char c;

    while (true) {
        c = input[forward];

        switch (state) {
            case 0: {
                if (isdigit(c)) { state = 1; forward++; }
                else { return false; }
                break;
            }

            case 1: {
                if (isdigit(c)) { state = 1; forward++; }
                else if (c == '.')   { state = 3; forward++; }
                else { state = 2; }
                break;
            }

            case 2: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(INT_CONST, "INT_CONST", lexeme);
                return true;
            }

            case 3: {
                if (isdigit(c)) { state = 1; forward++; }
                else { state = 4; }
                break;
            }

            case 4: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(FLOAT_CONST, "FLOAT_CONST", lexeme);
                return true;
            }
        }
    }
}


// Automato para reconhecer strings (coisas entre "")
bool LexicalAnalyzer::getString() {
    int state = 0;
    forward = lexeme_begin;
    char c;

    while (true) {
        c = input[forward];

        switch (state) {
            case 0: {
                if (c == '"') { state = 1; forward++; }
                else { return false; }
                break;
            }

            case 1: {
                if (c != '"') {
                    if (forward >= input.length()) {
                        return false;
                    } else {
                        forward++;
                    }
                }
                else {
                    state = 2;
                    forward++;
                }
                break;
            }

            case 2: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(STRING_CONST, "STRING_CONST", lexeme);
                return true;
            }
        }
    }
}


// Automato para reconhecer operadores
bool LexicalAnalyzer::getOperator() {
    int state = 0;
    forward = lexeme_begin;
    char c;

    while (true) {
        c = input[forward];

        switch (state) {
            case 0: {
                if (c == '+') { state = 1; forward++; }
                else if (c == '-') { state = 1; forward++; }
                else if (c == '*') { state = 1; forward++; }
                else if (c == '/') { state = 1; forward++; }
                else if (c == '%') { state = 1; forward++; }
                else if (c == '<') { state = 3; forward++; }
                else if (c == '>') { state = 3; forward++; }
                else if (c == '=') { state = 4; forward++; }
                else if (c == '!') { state = 4; forward++; }
                else { return false; }
                break;
            }

            case 1: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(ARITH_OPER, "ARITH_OPER", lexeme);
                return true;
            }

            case 2: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(RELAT_OPER, "RELAT_OPER", lexeme);
                return true;
            }

            case 3: {
                if (c == '=') forward++;
                state = 2;
                break;
            }

            case 4: {
                if (c == '=') { state = 2; forward++; }
                else { return false; }
                break;
            }
        }
    }
}


// Automato para reconhecer pontuacoes
bool LexicalAnalyzer::getPonctuation() {
    int state = 0;
    forward = lexeme_begin;
    char c;

    while (true) {
        c = input[forward];

        switch (state) {
            case 0: {
                if (c == '(') { state = 1; forward++; }
                else if (c == ')') { state = 2; forward++; }
                else if (c == '[') { state = 3; forward++; }
                else if (c == ']') { state = 4; forward++; }
                else if (c == '{') { state = 5; forward++; }
                else if (c == '}') { state = 6; forward++; }
                else if (c == ',') { state = 7; forward++; }
                else if (c == ';') { state = 8; forward++; }
                else if (c == '=') { state = 9; forward++; }
                else { return false; }
                break;
            }

            case 1: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(OPEN_PARENT, "OPEN_PARENT", lexeme);
                return true;
            }

            case 2: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(CLOSED_PARENT, "CLOSED_PARENT", lexeme);
                return true;
            }

            case 3: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(OPEN_SQRBRACK, "CLOSED_SQRBRACK", lexeme);
                return true;
            }

            case 4: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(CLOSED_SQRBRACK, "CLOSED_SQRBRACK", lexeme);
                return true;
            }

            case 5: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(OPEN_BRACES, "OPEN_BRACES", lexeme);
                return true;
            }

            case 6: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(CLOSED_BRACES, "CLOSED_BRACES", lexeme);
                return true;
            }

            case 7: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(COMMA, "COMMA", lexeme);
                return true;
            }

            case 8: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(SEMICOLON, "SEMICOLON", lexeme);
                return true;
            }

            case 9: {
                string lexeme = input.substr(lexeme_begin, forward-lexeme_begin);
                addToken(EQUAL, "EQUAL", lexeme);
                return true;
            }
        }
    }
}

// Verifica se o lexeme_begin esta apontado para um espaco em branco
bool LexicalAnalyzer::isWhiteSpace() {
    if (isspace(input[lexeme_begin])) {
        advanceLexemeBegin();
        return true;
    }
    return false;
}


// Funcao principal, executa a analise lexica
bool LexicalAnalyzer::analyze() {
    while (lexeme_begin < input.length()-1) {

        while (isWhiteSpace()) continue;

        if (getIdentifier()) { reachForward(); }
        else if (getNumber()) { reachForward(); }
        else if (getString()) { reachForward(); }
        else if (getOperator()) { reachForward(); }
        else if (getPonctuation()) { reachForward(); }
        else {
            print_tokens();
            errorHandler("unrecognized symbol");
            return false;
        }
    }
    return true;
}


// Exibe os tokens encontrados
void LexicalAnalyzer::print_tokens() {
    printf("----- Tokes -----\n");
    printf("%-20s | %-20s | %s\n", "Lexeme", "TokenType", "(Line, Column)");
    for (Token t : tokens) {
        printf("%-20s | %-20s | (%d, %d)\n", t.lexeme.c_str(), t.str_type.c_str(), t.line, t.column);
    }
    cout << endl;
}
