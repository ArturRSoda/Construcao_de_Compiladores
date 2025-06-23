#include "include/syntax_analyzer.hpp"

using namespace std;

SyntaxAnalyzer::SyntaxAnalyzer(vector<Token> v) {
    tokens = v;
    currentTokenIndex = 0;
}

// Load grammar from file
void SyntaxAnalyzer::loadGrammar(string filename) {
    ifstream file(filename);
    string line;
    bool firstRule = true;

    while (getline(file, line)) {
        if (line.empty()) continue;
        size_t arrow = line.find("->");
        if (arrow == string::npos) continue;

        string lhs = trim(line.substr(0, arrow));
        string rhs = line.substr(arrow + 3);

        nonTerminals.insert(lhs);

        if (firstRule) {
            startSymbol = lhs;
            firstRule = false;
        }

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

    for (const string& symbol : possibleSymbols) {
        if (!nonTerminals.count(symbol) && symbol != "ϵ") {
            terminals.insert(symbol);
        }
    }
}

// FIRST set computation
set<string> SyntaxAnalyzer::computeFirst(string symbol) {
    if (firstSet.count(symbol))
        return firstSet[symbol];

    set<string> result;

    // Terminal
    if (!nonTerminals.count(symbol)) {
        result.insert(symbol);
        return result;
    }

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

    firstSet[symbol] = result;
    return result;
}

void SyntaxAnalyzer::computeAllFirst() {
    for (auto& rule : grammar)
        computeFirst(rule.first);
}

// FOLLOW set computation
void SyntaxAnalyzer::computeFollow() {
    followSet[startSymbol].insert("$");

    bool changed;
    do {
        changed = false;
        for (auto& rule : grammar) {
            string A = rule.first;
            for (auto& production : rule.second) {
                for (size_t i = 0; i < production.size(); i++) {
                    string B = production[i];
                    if (!grammar.count(B)) continue; // Skip terminals

                    set<string> trailer;
                    bool epsilonFound = true;
                    for (size_t j = i + 1; j < production.size(); j++) {
                        set<string> firstNext = computeFirst(production[j]);
                        trailer.insert(firstNext.begin(), firstNext.end());
                        if (firstNext.find("ϵ") == firstNext.end()) {
                            epsilonFound = false;
                            break;
                        }
                    }

                    bool updated = false;
                    if (epsilonFound || i == production.size() - 1) {
                        size_t before = followSet[B].size();
                        followSet[B].insert(followSet[A].begin(), followSet[A].end());
                        if (followSet[B].size() > before)
                            updated = true;
                    }

                    trailer.erase("ϵ");
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
    for (auto& rule : grammar) {
        string A = rule.first;
        for (auto production : rule.second) {
            set<string> firstAlpha;
            bool derivesEpsilon = true;

            for (string symbol : production) {
                set<string> firstSym = computeFirst(symbol);
                firstAlpha.insert(firstSym.begin(), firstSym.end());
                if (firstSym.find("ϵ") == firstSym.end()) {
                    derivesEpsilon = false;
                    break;
                }
            }

            for (string terminal : firstAlpha) {
                if (terminal != "ϵ")
                    parseTable[A][terminal] = production;
            }

            if (derivesEpsilon || firstAlpha.find("ϵ") != firstAlpha.end()) {
                for (string terminal : followSet[A]) {
                    parseTable[A][terminal] = production;
                }
            }
        }
    }
}

Token SyntaxAnalyzer::getCurrentToken() {
    if (currentTokenIndex < tokens.size())
        return tokens[currentTokenIndex];
    else
        return Token{TokenType::END_OF_FILE, "END_OF_FILE", "$", -1, -1};  // Token fictício para EOF
}

void SyntaxAnalyzer::advanceToken() {
    if (currentTokenIndex < tokens.size())
        currentTokenIndex++;
}

// Parse input tokens
bool SyntaxAnalyzer::parse() {
    stack<string> stk;
    stk.push("$");
    stk.push(startSymbol);

    while (!stk.empty()) {
        string top = stk.top();
        Token token = getCurrentToken();
        string currentSymbol = getTerminalName(token);

        stk.pop();

        if (top == "$" && currentSymbol == "$") {
            cout << "Parsing successful!" << endl;
            return true;
        } else if (!nonTerminals.count(top)) {
            // Terminal
            if (top == currentSymbol) {
                advanceToken();
            } else {
                cout << "Syntax error at line " << token.line << ", column " << token.column << ": expected '" << top << "', got '" << currentSymbol << "'" << endl;
                return false;
            }
        } else {
            // Non-terminal
            if (parseTable[top].count(currentSymbol)) {
                auto production = parseTable[top][currentSymbol];
                auto it = production.rbegin();
                while (it != production.rend()) {
                    if (*it != "ϵ")
                        stk.push(*it);
                    ++it;
                }
            } else {
                cout << "Syntax error at line " << token.line << ", column " << token.column << ": no rule for '" << top << "' with lookahead '" << currentSymbol << "'" << endl;
                return false;
            }
        }
    }

    return false;
}

// Display First sets
void SyntaxAnalyzer::printFirstSets() {
    cout << "\n--- FIRST Sets ---" << endl;
    for (auto& p : firstSet) {
        cout << "First(" << p.first << ") = { ";
        for (string s : p.second) cout << s << " ";
        cout << "}" << endl;
    }
}

// Display Follow sets
void SyntaxAnalyzer::printFollowSets() {
    cout << "\n--- FOLLOW Sets ---" << endl;
    for (auto& p : followSet) {
        cout << "Follow(" << p.first << ") = { ";
        for (string s : p.second) cout << s << " ";
        cout << "}" << endl;
    }
}

// Display Parse Table
void SyntaxAnalyzer::printParseTable() {
    cout << "\n--- LL(1) Parsing Table ---" << endl;
    for (auto& row : parseTable) {
        string nonTerminal = row.first;
        for (auto& entry : row.second) {
            cout << "M[" << nonTerminal << ", " << entry.first << "] = ";
            for (string s : entry.second)
                cout << s << " ";
            cout << endl;
        }
    }
}

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
}

/*
int main() {
    loadGrammar("../docs/new_gramatica_LL1.txt");

    printGrammar();
    for (auto& rule : grammar)
        computeFirst(rule.first);

    computeFollow();
    buildParseTable();

    printFirstSets();
    printFollowSets();
    printParseTable();

    cout << "\nEnter input tokens (space-separated, e.g., 'id + id * id'): ";
    string line;
    getline(cin, line);
    vector<string> tokens = split(line);

    parseInput(tokens);

    return 0;
}
*/
