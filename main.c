#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
// #include "gadgets.h"
#include "utils.h"
#include "pseudo.h"

// Move var and related to utils - consider changing data structure
typedef struct Var {
    int value;
    char reg[4];
    char name[];
} Var;

typedef struct Vars {
    int count;
    Var* vars[];
} Vars;

Var* findVar(char* name, Vars* vars){
    for (int i = 0 ; i < vars->count ; i++){
        if(strcmp(vars->vars[i]->name, name) == 0){
            return vars->vars[i];
        }
    }
}

void createPseudo(int progLines, char** prog, Vars* vars, Pseudo* pseudoInst) {
    for (int i = 0 ; i < progLines ; i++){
            char* line = strdup(prog[i]);
            char* opcode = strtok(line, " ");
            char* operands = strtok(NULL, "");  // Save all operands
            char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
            int numOperands = getOperands(operandList, operands);
            
            if(strcmp(opcode,"Var") == 0){
                Var *newVar = malloc(sizeof(Var) + strlen(operandList[0]) + 1);
                strcpy(newVar->reg,"new");
                strcpy(newVar->name, operandList[0]);
                LoadConst newConst = {
                    operandList[0],
                    atoi(operandList[1])
                };
                Pseudo p = {
                    LOAD_CONST,
                    .loadConst = newConst
                };
                vars->vars[vars->count] = newVar;  // Use hastable? Also probs shouldn't save value yet??
                vars->count++;
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

// Assume can only load const with pop for now - DELETE?
// void findPossibleLoadRegisters(Var var, Gadgets gadgets){
//     for (int i = 0 ; i < gadgets.numLoadConstGadgets ; i++) {
//         var.regs[i] = gadgets.loadConstGadgets[i].operands[0];
//     }
// }

char* checkRegisterPossible(Var* var, char* reg, Gadgets gadgets){
    // Already in correct register
    if(strcmp(var->reg,reg) == 0){
        return "";  // No change needed
    }
    // Unloaded var
    else if(strcmp(var->reg,"new") == 0){
        for (int i = 0 ; i < gadgets.numLoadConstGadgets ; i++) {
            Gadget loadGadget = gadgets.loadConstGadgets[i];
            if (strcmp(loadGadget.operands[0],reg) == 0){
                return loadGadget.assembly;
            }
        }
        return NULL;  // Load not possible without move TODO
    }
    // Loaded but in wrong register
    else {
        // Move??
        return NULL;
    }
    return NULL;
}

// Create type a=a+b
void synthesiseAdd(ArithOp inst, Vars* vars, Gadgets gadgets){
    Var* a = findVar(inst.operand1,vars);
    Var* b = findVar(inst.operand2,vars);

    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++){
        Gadget gadget = gadgets.arithOpGadgets[i];
        if (strcmp(gadget.opcode,"add") == 0) {
            char* setupA = checkRegisterPossible(a, gadget.operands[0], gadgets); 
            char* setupB = checkRegisterPossible(b, gadget.operands[1], gadgets);
            if (setupA != NULL && setupB != NULL){
                // Set new register - could be the same if unchanged
                strcpy(a->reg, gadget.operands[0]);
                strcpy(b->reg, gadget.operands[1]);
                printf("%s\n%s\n%s\n",setupA,setupB,gadget.assembly);
            }
        }
    }
}

// return a list of registers currently in use
// char** usedRegisters(int num, Var* vars){
//     char* usedRegs[num];
//     for (int i = 0 ; i < num ; i++) {
//         usedRegs[i] = vars[i].reg;
//     }
//     return usedRegs;
// }

void translatePseudo(int progLines, Vars* vars, Pseudo* pseudoInst, Gadgets gadgets){
    for (int i = 0 ; i < progLines ; i++){
        // char** usedRegs = usedRegisters(progLines,)
        switch (pseudoInst[i].type){
            case LOAD_CONST:
            {
                // LoadConst inst = pseudoInst[i].loadConst;
                // Var var = findVar(inst.out,vars,progLines);
                // findPossibleLoadRegisters(var, gadgets);
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
    const int progLines = 5;
    char* prog[progLines] = {
        "Var x, 1",
        "Var y, 2",
        "Add x, y",
        "Var z, 3",
        "Add x, z"
    };
    Vars *vars = malloc(sizeof(Vars) + sizeof(Var*)*progLines);
    vars->count = 0;
    Pseudo pseudoInst[progLines];
    
    createPseudo(progLines, prog, vars, pseudoInst);
    Gadgets gadgets = loadGadgets();
    translatePseudo(progLines, vars, pseudoInst, gadgets);

    // printf("%d %d\n",vars[0].value,vars[1].value);
    // printf("%d\n",pseudoInst[0].type);
    return 0;
}