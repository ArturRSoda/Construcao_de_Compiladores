#include "include/syntax_analyzer.hpp"

#include <memory>

using namespace std;

SyntaxAnalyzer::SyntaxAnalyzer(vector<Token> v) {
    tokens = v;
    currentTokenIndex = 0;
}


// Load grammar from file
void SyntaxAnalyzer::loadGrammar(string filename) {
    // Abre arquivo da gramatica
    ifstream file(filename);
    string line;
    bool firstRule = true; // usado para setar o nao-terminal inicial (considera primeira producao)

    // Le arquivo linha por linha
    while (getline(file, line)) {
        // Ignora linhas em branco e linhas que nao tem '->'
        if (line.empty()) continue;
        size_t arrow = line.find("->");
        if (arrow == string::npos) continue;

        // Separa linha em strings de cabeca de producao e producoes
        string lhs = trim(line.substr(0, arrow));
        string rhs = line.substr(arrow + 3);

        // Adiciona cabeca de producao a lista de nao-terminais
        nonTerminals.insert(lhs);

        // Define simbolo inicial
        if (firstRule) {
            startSymbol = lhs;
            firstRule = false;
        }

        // Divide as producoes pelo '|'
        // E adiciona producoes
        istringstream ss(rhs);
        string prod;
        while (getline(ss, prod, '|')) {
            vector<string> production = split(prod);
            for (const string& symbol : production) {
                if (symbol != "ϵ" && !nonTerminals.count(symbol)) {
                    possibleSymbols.insert(symbol);
                }
            }
            grammar[lhs].push_back(production);
        }
    }

    // Depois de de ler o arquivo todo, define os terminais
    for (const string& symbol : possibleSymbols) {
        if (!nonTerminals.count(symbol) && symbol != "ϵ") {
            terminals.insert(symbol);
        }
    }
}


// FIRST set computation
set<string> SyntaxAnalyzer::computeFirst(string symbol) {
    // Se o conjunto FIRST do simbolo ja tiver sido computado
    // retorna o conjunto
    if (firstSet.count(symbol))
        return firstSet[symbol];

    // Cria conjunto first
    set<string> result;

    // Confere se simbulo eh terminal
    // se for retorna o conjunto com ele mesmo
    if (!nonTerminals.count(symbol)) {
        result.insert(symbol);
        return result;
    }

    // Passa por todas as producoes do nao-terminal
    // Para cada producao, processa os simbolos recursivamente
    for (auto production : grammar[symbol]) {
        bool epsilonInAll = true;
        for (string token : production) {
            set<string> firstOfToken = computeFirst(token);
            result.insert(firstOfToken.begin(), firstOfToken.end());
            if (firstOfToken.find("ϵ") == firstOfToken.end()) {
                epsilonInAll = false;
                break;
            }
        }
        if (!epsilonInAll)
            result.erase("ϵ");
    }

    // Salve o FIRST comptuado
    firstSet[symbol] = result;
    return result;
}


void SyntaxAnalyzer::computeAllFirst() {
    for (auto& rule : grammar)
        computeFirst(rule.first);
}


// FOLLOW set computation
void SyntaxAnalyzer::computeFollow() {
    // Adiciona '$' no non-terminal inicial
    followSet[startSymbol].insert("$");

    // Faz o loop ateh que nenhum conjunto follow mude
    bool changed;
    do {
        changed = false;

        // Passa por cada producao
        for (auto& rule : grammar) {
            string A = rule.first;
            for (auto& production : rule.second) {

                // Para cada simbulo nao-terminal da producao
                for (size_t i = 0; i < production.size(); i++) {
                    string B = production[i];
                    if (!grammar.count(B)) continue; // Pula terminais

                    // Para cada nao-terminal B, olha para os simbolos depois de B
                    set<string> trailer;
                    bool epsilonFound = true;
                    for (size_t j = i + 1; j < production.size(); j++) {
                        set<string> firstNext;
                        if (nonTerminals.count(production[j])) 
                            firstNext = firstSet[production[j]];
                        else
                            firstNext = { production[j] };

                        trailer.insert(firstNext.begin(), firstNext.end());
                        if (firstNext.find("ϵ") == firstNext.end()) {
                            epsilonFound = false;
                            break;
                        }
                    }

                    // Se B deriva epsilon ou B eh final de producao
                    // adiciona FOLLOW(A) em FOLLOW(B)
                    bool updated = false;
                    if (epsilonFound || i == production.size() - 1) {
                        size_t before = followSet[B].size();
                        followSet[B].insert(followSet[A].begin(), followSet[A].end());
                        if (followSet[B].size() > before)
                            updated = true;
                    }

                    trailer.erase("ϵ");

                    // Confere se algo mudou
                    size_t before = followSet[B].size();
                    followSet[B].insert(trailer.begin(), trailer.end());
                    if (followSet[B].size() > before)
                        updated = true;

                    if (updated)
                        changed = true;
                }
            }
        }
    } while (changed);
}


// Build LL(1) Parsing Table
void SyntaxAnalyzer::buildParseTable() {
    // Loop para cada producao
    for (auto& rule : grammar) {
        string A = rule.first;
        for (auto production : rule.second) {
            set<string> firstAlpha;
            bool derivesEpsilon = true;

            // Para cada simbolo da producao calcula o conjunto FIRST
            // E une com o conjunto First da cabeca
            for (string symbol : production) {
                set<string> firstSym;
                if (nonTerminals.count(symbol))
                    firstSym = firstSet[ symbol ];
                else
                    firstSym = { symbol };

                firstAlpha.insert(firstSym.begin(), firstSym.end());
                if (firstSym.find("ϵ") == firstSym.end()) {
                    derivesEpsilon = false;
                    break; // Stop if we find a non-epsilon production
                }
            }

            // Adiciona a producao na tabela para cada terminal do conjutno FIRST
            for (string terminal : firstAlpha) {
                if (terminal != "ϵ") {
                    // Apenas adiciona a producao se ainda nada foi adicionado
                    if (parseTable[A].count(terminal) == 0) {
                        parseTable[A][terminal] = production;
                    }
                }
            }

            // Se a producao deriva epsilon, adiciona a producao para os simbulos do conjunto FOLLOW
            if (derivesEpsilon) {
                for (string terminal : followSet[A]) {
                    // Apenas adiciona a producao se ainda nada foi adicionado
                    if (parseTable[A].count(terminal) == 0) {
                        parseTable[A][terminal] = production;
                    }
                }
            }
        }
    }
}


// Retorna o token apontado na lista de tokens ta analizador lexico
Token SyntaxAnalyzer::getCurrentToken() {
    if (currentTokenIndex < static_cast<int>(tokens.size()))
        return tokens[currentTokenIndex];
    else
        return Token{TokenType::END_OF_FILE, "END_OF_FILE", "$", -1, -1};
}

// Avanca o ponteiro da lista de token
void SyntaxAnalyzer::advanceToken() {
    if (currentTokenIndex < static_cast<int>(tokens.size()))
        currentTokenIndex++;
}

Node* make_partial_node(string grammar_name, Node* parent) {
    return new Node{grammar_name, {}, parent, {}};
}

// Parse os tokens da lista de tokens do analizador lexico
Node* SyntaxAnalyzer::parse() {
    // Inicializa a pilha
    vector<string> stk;
    stk.push_back("$");
    stk.push_back(startSymbol);

    Node* tree = make_partial_node(startSymbol, 0);
    Node* node = tree;

    // Continua olhando para a pilha enquanto nao vazia
    while (!stk.empty()) {
        // Get o topo da pilha e o token apontado

        //cout << "Pilha: {";
        //for (string& s : stk) {
        //    cout << "\"" << s << "\" ";
        //}
        //cout << "}\n";

        string top = stk.back();
        Token token = getCurrentToken();
        string currentSymbol = getTerminalName(token);

        stk.pop_back();

        bool pushed_to_stack = false;

        // Se chegar ao fundo da pilha e acabar os tokens de entrada -> sucesso
        if (top == "$" && currentSymbol == "$") {
            cout << "Parsing successful!" << endl;
            return tree;
        }
        // Se o topo da pilha for terminal
        else if (!nonTerminals.count(top)) {
            if (top == currentSymbol) {
                advanceToken();

                node->token = token;

                //node->children.push_back(new Node{token.lexeme, node, {}});
            } else {
                cout << "Syntax error at line " << token.line << ", column " << token.column << ": expected '" << top << "', got '" << currentSymbol << "'" << endl;
                return 0;
            }
        }
        // Se o topo da pilha for nao-terminal
        else {
            if (parseTable[top].count(currentSymbol)) {
                auto production = parseTable[top][currentSymbol];
                auto it = production.rbegin();

                while (it != production.rend()) {
                    if (*it != "ϵ") {
                        stk.push_back(*it);
                        pushed_to_stack = true;
                    }
                    ++it;
                }
                for (string& s : production) {
                    if (s != "ϵ") {
                        node->children.push_back(make_partial_node(s, node));
                    }
                }
                
                if (node->children.size()) {
                    node = node->children[0];
                }
            } else {
                cout << "Syntax error at line " << token.line << ", column " << token.column << ": no rule for '" << top << "' with lookahead '" << currentSymbol << "'" << endl;
                return 0;
            }
        }

        if (!pushed_to_stack) {
            Node* prev = 0;
            while (true) {
                if (!node) break;

                int i = 0;
                for (; i < (int)node->children.size(); i++) {
                    if (node->children[i] == prev) {
                        break;
                    }
                }
                bool more_child = (i+1 < (int)node->children.size());

                if (more_child) {
                    node = node->children[i+1];
                    break;
                }

                prev = node;
                node = node->parent;
            }
        }
    }

    return 0;
}


// Printa conjutos first
void SyntaxAnalyzer::printFirstSets() {
    cout << "\n--- FIRST Sets ---" << endl;
    for (auto& p : firstSet) {
        cout << "First(" << p.first << ") = { ";
        for (string s : p.second) cout << s << " ";
        cout << "}" << endl;
    }
    cout << endl;
}


// Printa conjuntos FOLLOW
void SyntaxAnalyzer::printFollowSets() {
    cout << "\n--- FOLLOW Sets ---" << endl;
    for (auto& p : followSet) {
        cout << "Follow(" << p.first << ") = { ";
        for (string s : p.second) cout << s << " ";
        cout << "}" << endl;
    }
    cout << endl;
}


// Printa Tabela
void SyntaxAnalyzer::printParseTable(string out_dir) {
    ofstream out_file;
    out_file.open(out_dir);
    if (!out_file.is_open()) {
        cerr << "Error while opening the output file for SymbolTable!";
    }

    out_file << "===================\n";
    out_file << "LL(1) Parsing Table" << endl;
    out_file << "===================\n";
    for (auto& row : parseTable) {
        string nonTerminal = row.first;
        for (auto& entry : row.second) {
            out_file << "M[" << nonTerminal << ", " << entry.first << "] = ";
            for (string s : entry.second)
                out_file << s << " ";
            out_file << endl;
        }
    }
    out_file << endl;

    out_file.close();
    cout << "Parse Table was successifully saved on " << out_dir << endl;
}

// Printa gramatica
void SyntaxAnalyzer::printGrammar() {
    cout << "\n--- Loaded Grammar ---\n";
    for (auto& rule : grammar) {
        cout << rule.first << " -> ";
        for (auto& prod : rule.second) {
            for (string s : prod) cout << s << " ";
            cout << " | ";
        }
        cout << endl;
    }
    cout << endl;
}

