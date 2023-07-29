#include <iostream>
#include <fstream>
#include <string.h> // What is the difference between #include <string> and #include <string.h>
#include <cctype>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>


using namespace std;

char line[512];
char delim[] = "\t ";
int line_no = 0;

int token_no = 0;
char *token;

vector<string> symbol;
vector<int> symbol_val; 
int module_number = 0; //Counting the number of instructions to update module number 

vector<string> memory_table;
vector<int> memory_val; 
unordered_map<int, string> map_idx_str;
unordered_map<string, int> map_str_pos;

int eof = 0;

// Rule 4
int mod_ctr = 0;
unordered_map<string, int> war_mod_def_use;

// Rule 7
unordered_map<string, int> rule7;
string formatted_ss;
int map_val = 0;

// Rule 5
unordered_map<string, int> rule5_val;

// Rule 2 
unordered_map<string, string> rule_symbol;

// Parse error
int token_line_no;
int prev_token_idx;
int spaceCount = 0;

// pass1 warnings
vector<string> pass1_warn;

void getToken(ifstream &inputFile){
    // Read next line if end of line
    // Corner case is end of file
    if (inputFile.eof()){
        eof = -1;
        if (token == NULL){
            line_no = line_no - 1; 
            if (line_no == token_line_no){
                token_no = prev_token_idx;
            }
            else{
                token_no = spaceCount;
            }
        }
    }
    else{
        if (token != NULL){
            token  = strtok(NULL, delim);
            spaceCount = 1;
            }

        if (token == NULL) {
        // else read the next token 
            inputFile.getline(line, 512); // This gets the first line for the input
            for (int i = 0; line[i] != '\0'; i++) {
                if (line[i] == ' ') {
                    spaceCount++;
                }
            }
            line_no = line_no + 1;
            // cout << line << "\n";
            token = strtok(line, delim);
        }
        token_no = token - line + 1;
    }
    if ((token == NULL) && (eof != -1)){
        getToken(inputFile);
    }

    if (token != NULL){
        token_line_no = line_no;
        prev_token_idx = token_no + strlen(token);
    }

    // printf ("Token %d:%d %s\n", line_no, token_no, token);
}

int readInt(ifstream &inputFile){
    getToken(inputFile);
    // printf ("Inside readInt\n");
    // lets say your input is 1111 or 1a1a
    // run a loop to the end of token_length if everything passes through then we can convert and return
    // printf (token);

    //End of the file case
    if ((eof == -1) || (token == NULL)){
        return -1;
    }

    int n = strlen(token);
    for (int i=0; i < n; i++){
        if (!isdigit(token[i])){
            printf ("Parse Error line %d offset %d: NUM_EXPECTED\n", line_no, token_no);
            exit (1);}
    }
    int num = atoi(token);
    return num;
}

char* readSym(ifstream &inputFile){
    getToken(inputFile);

    //End of the file case
    if (eof == -1){
        printf ("Parse Error line %d offset %d: SYM_EXPECTED", line_no, token_no);
        exit (1);
    }

    int n = strlen(token);
    // printf ("N is %d\n", n);
    if (n>16){            
        printf ("Parse Error line %d offset %d: SYM_TOO_LONG", line_no, token_no);
        exit (1);
    }
    int dig = 0;
    for (int i=0; i < n; i++){
        if (!isalnum(token[i])){
            printf ("Parse Error line %d offset %d: SYM_EXPECTED", line_no, token_no);
            exit (1);}

        else{
            if (isdigit(token[i])){
                if (i == 0){
                    printf ("Parse Error line %d offset %d: SYM_EXPECTED", line_no, token_no);
                    exit (1);
                }
                dig++;
                }
        }
    }
    if (dig == n){
        printf ("Parse Error line %d offset %d: SYM_EXPECTED", line_no, token_no);
        exit (1);
    }

    else{
        return token;
    }
}



char* readIEAR(ifstream &inputFile){
    getToken(inputFile);

    //End of the file case
    if (eof == -1){
        printf ("Parse Error line %d offset %d: ADDR_EXPECTED", line_no, token_no);
        exit (1);
    }

    if ((strcmp(token, "A") == 0) || (strcmp(token, "E") == 0) || (strcmp(token, "I") == 0) || (strcmp(token, "R") == 0)){
        // printf ("IEAR is %s\n", token);
        return token;
        }

    else{
        printf("Parse Error line %d offset %d: ADDR_EXPECTED\n", line_no, token_no);
        exit(1);
    }
    
}

void createSymbol(string sym_fun, int val_fun){
    symbol.push_back(sym_fun);
    symbol_val.push_back(val_fun);
}


void createMap(int val_fun){
    // memory_table.push_back(sym_fun);
    memory_val.push_back(val_fun);
}


void Pass1(ifstream &inputFile) {
    int mod_ctr = 0;
    while (!inputFile.eof()) {
        mod_ctr++;
        int rule2_flag = 0;
        rule5_val.clear();

        int defcount = readInt(inputFile);

        // END OF FILE CASE
        if ((token == NULL) && (eof!=-1)){
            defcount = readInt(inputFile);
        }

        if (eof == -1){
            return;
        }
        

        if (defcount>16){
            printf ("Parse Error line %d offset %d: TOO_MANY_DEF_IN_MODULE", line_no, token_no);
            exit (1);
        }

        for (int i=0;i<defcount;i++) {
            string sym1 = string(readSym(inputFile));
            // printf ("definition is %s ", sym1.c_str());
            int val1 = readInt(inputFile);
            for (int m = 0; m<symbol.size(); m++){
                if (sym1 == symbol[m]){
                    rule2_flag = 1;
                    rule_symbol[sym1] = "Error: This variable is multiple times defined; first value used";
                    pass1_warn.push_back("Warning: Module " + to_string(mod_ctr) + ": " + sym1 + " redefined and ignored");
                }
            }
            if (rule2_flag == 0){
                createSymbol(sym1,val1 + module_number);
                rule5_val[sym1] = val1;
            }
            }


        // USELIST
        int usecount = readInt(inputFile);
        if (usecount==-1){
            printf ("Parse Error line %d offset %d: NUM_EXPECTED", line_no, token_no);
            exit (1);
        }
        if (usecount>16){
            printf ("Parse Error line %d offset %d: TOO_MANY_USE_IN_MODULE", line_no, token_no);
            exit (1);
        }
        for (int i=0;i<usecount;i++) {
            char* sym2 = readSym(inputFile);}


        // INSTRUCTION LIST
        int instcount = readInt(inputFile);
        if (instcount==-1){
            printf ("Parse Error line %d offset %d: NUM_EXPECTED", line_no, token_no);
            exit (1);
        }
        int prev_module_number = module_number;
        module_number = module_number + instcount;
        if (module_number>512){
            printf("Parse Error line %d offset %d: TOO_MANY_INSTR\n", line_no, token_no);
            exit (1);
        }
        for (int i=0;i<instcount;i++) {
            char* addressmode = readIEAR(inputFile);
            int operand = readInt(inputFile);
            // printf ("Address Mode %s:%d\n", addressmode, operand);
            }

        // Module 1, 2, 3 are processed 
        for (const auto& [k, v] : rule5_val){
            if (v>=instcount){
                printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n", mod_ctr, k.c_str(), v, instcount - 1);
                for (int j = 0; j<symbol.size(); j++){
                    if (symbol[j] == k){
                        symbol_val[j] = prev_module_number;
                    }
                }
            }
        }

        
        }
}


void Pass2(ifstream &inputFile) {
    while (!inputFile.eof()) {
        rule7.clear();
        int idx = 0;
        int defcount = readInt(inputFile);

        // END OF FILE CASE
        if ((token == NULL) && (eof!=-1)){
            defcount = readInt(inputFile);
        }

        if (eof == -1){
            return;
        }

        mod_ctr = mod_ctr + 1;
        for (int i=0;i<defcount;i++) {
            string sym1 = string(readSym(inputFile));
            int val1 = readInt(inputFile);
            if (war_mod_def_use.find(sym1) == war_mod_def_use.end()){
                war_mod_def_use[sym1] = mod_ctr;
            }
            }


        // USELIST
        int usecount = readInt(inputFile);
        for (int i=0;i<usecount;i++) {
            // printf ("In USELIST\n");
            char* sym2 = readSym(inputFile); // Read xy
            string str_sym2 = string(sym2); // Converted xy to string
            rule7[str_sym2] = mod_ctr;
            war_mod_def_use[str_sym2] = -1;
            // Check if xy is in map_str_pos
            if (map_str_pos.find(str_sym2)==map_str_pos.end()){ // if its not there 
                map_idx_str[idx] = string(sym2); // Add it to map_idx_str and also find its value 
                idx = idx + 1;

                for (int j = 0; j<symbol.size(); j++){
                    // printf ("%s \n", symbol[j].c_str());
                    if (symbol[j] == str_sym2){
                        map_str_pos[str_sym2] = symbol_val[j];
                    }
                }

            }
            }


        // INSTRUCTION LIST
        int instcount = readInt(inputFile);
        for (int i=0;i<instcount;i++) {
            string s;
            string addressmode = string(readIEAR(inputFile));
            int instr = readInt(inputFile);

            // Adressing model I
            if (addressmode == "I"){
                if (instr>=10000){
                    instr = 9999;
                    s = "Error: Illegal immediate value; treated as 9999";
                }
            }

            // Adressing model E
            if (addressmode == "E"){

                int opcode = instr%1000;
                int operand = instr - opcode;
                // printf("%d: %d: %s: %d\n", operand, opcode, map_idx_str[opcode].c_str(), map_str_pos[map_idx_str[opcode]]);
                rule7[map_idx_str[opcode]] = -1;
                // rule 6
                if (opcode>usecount-1){
                    s = "Error: External address exceeds length of uselist; treated as immediate";
                    instr = opcode + operand;
                }
                else{
                    if (map_str_pos.find(map_idx_str[opcode]) == map_str_pos.end()){
                        s = "Error: " + map_idx_str[opcode] +" is not defined; zero used";
                    }
                    else{
                        opcode = map_str_pos[map_idx_str[opcode]];
                        instr = operand + opcode;
                        }
                }
            }

            // Adressing model A
            if (addressmode == "A"){
                if (instr%1000>=512){
                    s = "Error: Absolute address exceeds machine size; zero used";
                    instr = instr - instr%1000;

                }
                if (instr/1000>=10){
                    s = "Error: Illegal opcode; treated as 9999";
                    instr = 9999;
                }
            }

            // Adressing model R
            if (addressmode == "R"){
                if (instr%1000>instcount){
                    instr = instr - instr%1000;
                    s = "Error: Relative address exceeds module size; zero used";
                }
                instr = instr + module_number;

                if (instr/1000>=10){
                    s = "Error: Illegal opcode; treated as 9999";
                    instr = 9999;
                }
                
            }
            createMap(instr);
            // Printing pass2
            stringstream ss;
            stringstream ss_instr;

            ss << setw(3) << setfill('0') << map_val;
            ss_instr << setw(4) << setfill('0') << instr;
            formatted_ss = ss.str();
            printf ("%s: %s ", formatted_ss.c_str(), ss_instr.str().c_str());
            printf ("%s\n", s.c_str());
            map_val++;
            }
        module_number = module_number + instcount;
        map_str_pos.clear();
        map_idx_str.clear();
        for (const auto& [k, v] : rule7) {
            if (v!=-1){
                printf ("Warning: Module %d: %s appeared in the uselist but was not actually used\n", v, k.c_str());
            }
        }

        }
}




int main(int argc, char* argv[]){
    string path = argv[1];
    char *token;

    // Read the input file - Where to read the file?
    ifstream inputFile;
    inputFile.open(path);

    // PASS1
    Pass1(inputFile);
    for (int i = 0; i<pass1_warn.size(); i++){
        printf("%s\n", pass1_warn[i].c_str());

    }
    // SYMBOL TABLE
    printf ("Symbol Table\n");
    int sym_size = symbol.size();
    string k;
    int v;
    for (int i = 0; i < sym_size; ++i) {
        k = symbol[i];
        v = symbol_val[i];
        printf ("%s=%d ", k.c_str(), v);
        printf ("%s\n", rule_symbol[k].c_str());
        }

    //REOPEN AND CLOSE FILE
    inputFile.close();
    inputFile.open(path);

    //PASS2
    eof = 0;
    module_number = 0;
    printf ("\nMemory Map\n");
    Pass2(inputFile);

    // Post pass 2 warnings 
    // war_mod_def_use
    printf ("\n");
    for (const auto& [k, v] : war_mod_def_use) {
        if (v!=-1){
            printf ("Warning: Module %d: %s was defined but never used\n", v, k.c_str());
        }
    }
    inputFile.close();
}