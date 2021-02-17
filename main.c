#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
// #include "gadgets.h"
#include "utils.h"
#include "pseudo.h"

typedef struct Var {
    char* name;
    int value;
    char** regs;
} Var;

Var findVar(char* name, Var* vars, int num){
    for (int i = 0 ; i < num ; i++){
        if(strcmp(vars[i].name, name) == 0){
            return vars[i];
        }
    }
}

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
                    .regs = malloc(20*3*sizeof(char))
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

// Assume can only load const with pop for now
void findPossibleLoadRegisters(Var var, Gadgets gadgets){
    for (int i = 0 ; i < gadgets.numLoadConstGadgets ; i++) {
        var.regs[i] = gadgets.loadConstGadgets[i].operands[0];
    }
}

bool checkRegisterPossible(Var var, char* reg){
    for (int i = 0 ; i < 20 ; i++){
        if (var.regs[i] == NULL){
            return false;
        }
        if (strcmp(var.regs[i],reg) == 0) {
            return true;
        }
    }
    return false;
}

// Create type a+a+b
void synthesiseAdd(ArithOp inst, Var* vars, Gadgets gadgets){
    Var a = findVar(inst.operand1,vars,3);
    Var b = findVar(inst.operand2,vars,3);

    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++){
        Gadget gadget = gadgets.arithOpGadgets[i];
        if (strcmp(gadget.opcode,"add") == 0) {
            if( checkRegisterPossible(a, gadget.operands[0]) && 
                checkRegisterPossible(b, gadget.operands[1]) ) {
                    printf("%s\n",gadget.assembly);
                }
        }
    }
}

void translatePseudo(int progLines, Var* vars, Pseudo* pseudoInst, Gadgets gadgets){
    for (int i = 0 ; i < progLines ; i++){
        switch (pseudoInst[i].type){
            case LOAD_CONST:
            {
                LoadConst inst = pseudoInst[i].loadConst;
                Var var = findVar(inst.out,vars,progLines);
                findPossibleLoadRegisters(var, gadgets);
                break;
            }
            case ARITH_OP:
            {
                ArithOp inst = pseudoInst[i].arithOp;
                switch (inst.opcode){
                    case '+': 
                        synthesiseAdd(inst, vars, gadgets);
                        break;
                }
                break;
            }
            default:
                break;
        }
    }
}

int main(){
    int progLines = 3;
    char* prog[3] = {
        "Var x, 1",
        "Var y, 2",
        "Add x, y"
    };
    Var vars[progLines];
    Pseudo pseudoInst[progLines];
    
    createPseudo(progLines, prog, vars, pseudoInst);
    Gadgets gadgets = loadGadgets();
    translatePseudo(progLines, vars, pseudoInst, gadgets);

    // printf("%d %d\n",vars[0].value,vars[1].value);
    // printf("%d\n",pseudoInst[0].type);
    return 0;
}