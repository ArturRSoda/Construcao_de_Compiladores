#include "include/intermediate_code_gen.hpp"

string IntermediateCodeGen::nextT() {
    static int i = 0;
    return "t" + to_string(i++);
}

string IntermediateCodeGen::nextLabel() {
    static int i = 0;
    return "label" + to_string(i++);
}

void IntermediateCodeGen::generateIntermediateCodeDfs(Node* node, IntermData* data) {
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

            data->code += expression_data.code
                        + "if False " + expression_data.t_out + " goto " + else_label + "\n"
                        + statement_data.code
                        + "goto " + end_label + "\n"
                        + else_label + ":\n"
                        + else_data.code
                        + end_label + ":\n";
        } else {
            string end_label = nextLabel();

            data->code += expression_data.code
                        + "if False " + expression_data.t_out + " goto " + end_label + "\n"
                        + statement_data.code
                        + end_label + ":\n";
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

        data->code += atribstat_init_data.code
                    + start_label + ":\n"
                    + expression_data.code
                    + "if False " + expression_data.t_out + " goto " + end_label + "\n"
                    + statement_data.code
                    + atribstat_inc_data.code
                    + "goto " + start_label + "\n"
                    + end_label + ":\n";

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

string IntermediateCodeGen::generateIntermediateCode(Node* tree) {
    IntermData data{};
    generateIntermediateCodeDfs(tree, &data);
    return data.code;
}

