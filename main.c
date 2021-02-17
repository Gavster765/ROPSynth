#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include "gadgets.h"
#include "utils.h"
#include "pseudo.h"

typedef struct Var {
    char* name;
    int value;
} Var;

void createPseudo(int progLines, char** prog, Var* vars, Pseudo* pseudoInst) {
    for (int i = 0 ; i < progLines ; i++){
            char* line = strdup(prog[i]);
            char* opcode = strtok(line, " ");
            char* operands = strtok(NULL, "");  // Save all operands
            char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
            int numOperands = getOperands(operandList, operands);
            
            if(strcmp(opcode,"Var") == 0){
                Var newVar = {
                    operandList[0],
                    // atoi(operandList[1])
                };
                LoadConst newConst = {
                    operandList[0],
                    atoi(operandList[1])
                };
                Pseudo p = {
                    LOAD_CONST,
                    .loadConst = newConst
                };
                vars[i] = newVar;  // Use hastable? Also probs shouldn't save value yet??
                pseudoInst[i] = p;
            }
            else if(strcmp(opcode,"Add") == 0){
                ArithOp newArith = {
                    '+',
                    operandList[0],
                    operandList[0],
                    operandList[1]
                };
                Pseudo p = {
                    ARITH_OP,
                    .arithOp = newArith
                };
                pseudoInst[i] = p;
            }
        }
}

int main(){
    // Gadgets gadgets = loadGadgets();
    int progLines = 3;
    char* prog[3] = {
        "Var x, 1",
        "Var y, 2",
        "Add x, y"
    };
    Var vars[progLines];
    Pseudo pseudoInst[progLines];
    
    createPseudo(progLines, prog, vars, pseudoInst);
    
    printf("%d %d\n",vars[0].value,vars[1].value);
    printf("%d\n",pseudoInst[0].type);
    return 0;
}