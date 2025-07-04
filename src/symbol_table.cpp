#include "include/symbol_table.hpp"

using namespace std;

// Construtor tabela de simbolos
SymbolTable::SymbolTable() {
    id = 0;
}


// Verifica se um lexema ja existe na tabela
bool SymbolTable::exists(string lexeme) {
    return table.find(lexeme) != table.end();
}


// Insere palavras-chave pre-definidas na tabela
void SymbolTable::insertKeyWord(TokenType type, string str_type, string lexeme) {
    Symbol s = {
        type,
        str_type,
        lexeme,
        {}       // Lista de ocorrências vazia inicialmente
    };
    table[lexeme] = s;
}


// Insere um novo simbolo ou atualiza ocorrencias se ja existir
void SymbolTable::insert(TokenType type, string str_type, string lexeme, int line, int column) {
    if (!exists(lexeme)) {
        Symbol s = {
            type,
            str_type,
            lexeme,
            {{line, column}}
        };
        table[lexeme] = s;
    }
    else {
        table[lexeme].occorrences.push_back({line, column});
    }
}


// Busca um simbolo na tabela e retorna seu ponteiro (nullptr se nao encontrado)
Symbol* SymbolTable::lookup(string lexeme) {
    auto it = table.find(lexeme);
    if (it != table.end())
        return &it->second;
    else
        return nullptr;
}


// Exibe a tabela de simbolos formatada
void SymbolTable::print() {
    cout << "===================\n";
    cout << "SYMBOL TABLE" << endl;
    cout << "===================\n";
    printf("%-20s | %-20s | %s\n", "lexeme", "TokenType", "occurrences (line, column)");
    for (auto x : table) {
        printf("%-20s | %-20s | ", x.first.c_str(), x.second.str_type.c_str());
        for (auto y: x.second.occorrences) {
            cout << "(" << y.first << ", " << y.second << ") ";
        }
        cout << endl;
    }
}
