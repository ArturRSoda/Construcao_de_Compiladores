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
void SymbolTable::print(string out_dir) {

    ofstream out_file;
    out_file.open(out_dir);
    if (!out_file.is_open()) {
        cerr << "Error while opening the output file for SymbolTable!";
    }

    out_file << "===================\n";
    out_file << "SYMBOL TABLE" << endl;
    out_file << "===================\n";
    out_file << left;
    out_file << setw(20) << "lexeme" << " | " << setw(20) << "TokenType" << " | " << "occurrences (line, column)" << endl;
    for (auto x : table) {
        out_file << setw(20) << x.first.c_str() << " | " << setw(20) << x.second.str_type.c_str() << " | ";
        for (auto y: x.second.occorrences) {
            out_file << "(" << y.first << ", " << y.second << ") ";
        }
        out_file << endl;
    }

    out_file.close();
    cout << "Symbol Table was successfully saved on " << out_dir << endl;
}
