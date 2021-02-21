#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
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
    return NULL;
}

Var* findVarByReg(char* reg, Vars* vars){
    for (int i = 0 ; i < vars->count ; i++){
        if(strcmp(vars->vars[i]->reg, reg) == 0){
            return vars->vars[i];
        }
    }
    return NULL;
}

Vars* copyVars(Vars* vars){
    Vars* copy = malloc(sizeof(Vars) + sizeof(Var*)*vars->count);
    copy->count = vars->count;
    for (int i = 0 ; i < vars->count ; i++){
        Var* var = vars->vars[i];
        Var *newVar = malloc(sizeof(Var) + strlen(var->name) + 1);
        strcpy(newVar->name, var->name);
        strcpy(newVar->reg, var->reg);
        newVar->value = var->value;
        copy->vars[i] = newVar;
    }
    return copy;
}

void freeVars(Vars* vars){
    for (int i = 0 ; i < vars->count ; i++){
        free(vars->vars[i]);
    }
    free(vars);
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

bool exists(char* reg, char** usedRegs, int count){
    for (int i = 0 ; i < count ; i++){
        if(strcmp(usedRegs[i],reg) == 0){
            return true;
        }
    }
    return false;
}

// return a list of registers currently in use
char** usedRegisters(Vars* vars){
    char** usedRegs = malloc(vars->count * sizeof(char*));
    for (int i = 0 ; i < vars->count ; i++) {
        usedRegs[i] = malloc(4);
        strcpy(usedRegs[i], vars->vars[i]->reg);
    }
    return usedRegs;
}

void freeUsedRegs(char** usedRegs, int count){
    for (int i = 0 ; i < count ; i++) {
        free(usedRegs[i]);    
    }
    free(usedRegs);
}

void addRegToUsed(char** used, char* reg, int count){
    if(!exists(reg,used,count)){
        for (int i = 0 ; i < count ; i++) {
            if(strcmp(used[i],"new") == 0) {
                strcpy(used[i],reg);
                return;
            }
        }
    }
}

void removeRegFromUsed(char** used, char* reg, int count){
    if(exists(reg,used,count)){
        for (int i = 0 ; i < count ; i++) {
            if(strcmp(used[i],reg) == 0) {
                strcpy(used[i],"new");
                return;
            }
        }
    }
}

char* moveRegAnywhere(char* src, char** usedRegs, Vars* vars, Gadgets gadgets) {
    for (int i = 0 ; i < gadgets.numMoveRegGadgets ; i++) {
        Gadget moveGadget = gadgets.moveRegGadgets[i];
        if (strcmp(src, moveGadget.operands[1]) == 0 &&
            !exists(moveGadget.operands[0], usedRegs, vars->count)) {
                Var* var = findVarByReg(src, vars);
                strcpy(var->reg, moveGadget.operands[0]);
                removeRegFromUsed(usedRegs, src, vars->count);  // Not required atm
                addRegToUsed(usedRegs, var->reg, vars->count);
                return moveGadget.assembly;
        }
    }
    return NULL;
}

char* moveReg(Var* var, char* dest, char** usedRegs, Vars* vars, Gadgets gadgets){
    // WARNING assumes no move gagdet longer than first
    char* assembly = malloc(2 * strlen(gadgets.moveRegGadgets[0].assembly) + 2);
    assembly[0] = '\0';    
    if (exists(dest, usedRegs, vars->count)){
        char* moveExisting = moveRegAnywhere(dest, usedRegs, vars, gadgets);
        if (moveExisting == NULL){
            return NULL;
        }
        strcat(assembly,moveExisting);
        strcat(assembly,"\n");
    }
    for (int i = 0 ; i < gadgets.numMoveRegGadgets ; i++) {
        Gadget moveGadget = gadgets.moveRegGadgets[i];
        if (strcmp(dest, moveGadget.operands[0]) == 0 &&
            strcmp(var->reg, moveGadget.operands[1]) == 0) {
                removeRegFromUsed(usedRegs, var->reg, vars->count);
                strcpy(var->reg,dest);
                strcat(assembly, moveGadget.assembly);
                addRegToUsed(usedRegs, dest, vars->count);
                return assembly;
        }
    }
    return NULL;
}

char* checkRegisterPossible(Var* var, char* dest, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;
    int count = vars->count;
    // Already in correct register
    if(strcmp(var->reg,dest) == 0){
        return "";  // No change needed
    }
    // Unloaded var
    else if(strcmp(var->reg,"new") == 0){
        for (int i = 0 ; i < gadgets.numLoadConstGadgets ; i++) {
            Gadget loadGadget = gadgets.loadConstGadgets[i];
            if (exists(dest,usedRegs,count)){
                break;  // Reg in use - would clobber value - move first?
            }
            else if (strcmp(loadGadget.operands[0],dest) == 0){
                strcpy(var->reg, dest);
                addRegToUsed(usedRegs, dest, vars->count);
                return loadGadget.assembly;
            }
        }
        for (int i = 0 ; i < gadgets.numLoadConstGadgets ; i++) {
            Gadget loadGadget = gadgets.loadConstGadgets[i];
            char* gadgetDest = loadGadget.operands[0];
            if (!exists(gadgetDest,usedRegs,count)){
                Vars* tmpVars = copyVars(vars);
                Var* tmpVar = findVar(var->name, tmpVars);
                strcpy(tmpVar->reg,gadgetDest);  // Temporially load
                char** tmpUsedRegs = usedRegisters(tmpVars);
                char* possMove = moveReg(tmpVar, dest, tmpUsedRegs, tmpVars, gadgets);
                if (possMove != NULL){
                    *varsPtr = tmpVars;
                    freeVars(vars);
                    *usedRegsPtr = tmpUsedRegs;
                    freeUsedRegs(usedRegs, count);
                    // WARNING memory leak!
                    char* assembly = malloc(strlen(loadGadget.assembly)
                                                + strlen(possMove) + 2);
                    assembly[0] = '\0';                                             
                    strcat(assembly,loadGadget.assembly);
                    strcat(assembly,"\n");
                    strcat(assembly,possMove);
                    return assembly;
                }
                freeVars(tmpVars);
                freeUsedRegs(tmpUsedRegs, count);
            }
        }
        return NULL;  // Load not possible without move TODO
    }
    // Loaded but in wrong register
    else {
        // Reg assigned but not correct
        // if (new) // merge
        //     generate list of all poss loads ??s
        
        // get moves with correct dest 
        // try to match with src <- even if requires a move?


        return moveReg(var, dest, usedRegs, vars, gadgets);
    }
    return NULL;
}

// Create type a=a+b
void synthesizeAdd(ArithOp inst, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    int count = vars->count;

    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++){
        char** usedRegs = usedRegisters(vars);
        Vars* tmpVars = copyVars(vars);
        Var* a = findVar(inst.operand1,tmpVars);
        Var* b = findVar(inst.operand2,tmpVars);
        Gadget gadget = gadgets.arithOpGadgets[i];
        if (strcmp(gadget.opcode,"add") == 0) {
            char* setupA = checkRegisterPossible(a, gadget.operands[0], &usedRegs, &tmpVars, gadgets); 
            // addRegToUsed(usedRegs,gadget.operands[0],count);
            // WARNING may have moved value from add dest to add src - could now be stuck when other moves where possible
            char* setupB = checkRegisterPossible(b, gadget.operands[1], &usedRegs, &tmpVars, gadgets);
            if (setupA != NULL && setupB != NULL){
                // Set new register - could be the same if unchanged
                // strcpy(a->reg, gadget.operands[0]);
                // strcpy(b->reg, gadget.operands[1]);
                *varsPtr = tmpVars;
                freeVars(vars);
                freeUsedRegs(usedRegs, count);
                printf("%s\n%s\n%s\n",setupA,setupB,gadget.assembly);
                return;
            }
        }
        freeUsedRegs(usedRegs, count);
        freeVars(tmpVars);
    }
}

void translatePseudo(int progLines, Vars* vars, Pseudo* pseudoInst, Gadgets gadgets){
    for (int i = 0 ; i < progLines ; i++){
        
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
                        synthesizeAdd(inst, &vars, gadgets);
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