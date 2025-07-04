#include "syntax_analyzer.hpp"

#include <string>
#include <vector>

using namespace std;

class IntermediateCodeGen {
public:
    string generateIntermediateCode(Node* tree);

private:
    struct IntermData {
        string code;

        string last_attribution;
        string next;
        int arg_count;

        string t_in;
        string t_out;
    };

    string nextT();
    string nextLabel();
    void generateIntermediateCodeDfs(Node* node, IntermData* data);

};
