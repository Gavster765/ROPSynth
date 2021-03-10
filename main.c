#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "gadgets.h"
#include "utils.h"
#include "pseudo.h"
#include "var.h"

void createPseudo(int progLines, char** prog, Vars* vars, Pseudo* pseudoInst) {
    Comp *currIf;  // Currently open if - TODO nested?
    bool loop = false;

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
                newVar->lifeSpan = i+1;
                newVar->loop = false;
                newVar->constant = true;  // Is constant until changed
                newVar->inMemory = false;
                newVar->memAddress = vars->count;  //  Not in memory
                LoadConst newConst = {
                    .out = operandList[0],
                    .value = atoi(operandList[1])
                };
                Pseudo p = {
                    .type = LOAD_CONST,
                    .loadConst = newConst
                };
                vars->vars[vars->count] = newVar;  // Use hastable?
                vars->count++;
                pseudoInst[i] = p;
            }
            else if(strcmp(opcode,"Add") == 0 || strcmp(opcode,"Sub") == 0){
                ArithOp newArith = {
                    .out = operandList[0],
                    .operand1 = operandList[0],
                    .operand2 = operandList[1]
                };

                if(strcmp(opcode,"Add") == 0) {
                    newArith.opcode = '+';
                }
                else if(strcmp(opcode,"Sub") == 0) {
                    newArith.opcode = '-';
                }

                Pseudo p = {
                    .type = ARITH_OP,
                    .arithOp = newArith
                };
                pseudoInst[i] = p;
                updateLifespan(operandList[0], vars, i, loop);
                updateLifespan(operandList[1], vars, i, loop);
            }
            else if(strcmp(opcode,"If") == 0) {
                updateLifespan(operandList[0], vars, i, loop);
                updateLifespan(operandList[2], vars, i, loop);
                
                Comp c = {
                    .opcode = operandList[1],
                    .operand1 = operandList[0],
                    .operand2 = operandList[2],
                    .end = i+1,  // Default end to next line
                    .joinedIf = NULL
                };

                Pseudo p = {
                    .type = COMP,
                    .comp = c
                };
                pseudoInst[i] = p;
                currIf = &pseudoInst[i].comp;
            }
            else if(strcmp(opcode,"ElseIf") == 0) {
                updateLifespan(operandList[0], vars, i, loop);
                updateLifespan(operandList[2], vars, i, loop);
                currIf->end = i;
                
                Comp c = {
                    .opcode = operandList[1],
                    .operand1 = operandList[0],
                    .operand2 = operandList[2],
                    .end = i+1,  // Default end to next line
                    .joinedIf = currIf
                };
                
                Pseudo p = {
                    .type = COMP,
                    .comp = c
                };
                
                pseudoInst[i] = p;
                currIf = &pseudoInst[i].comp;
            }
            else if(strcmp(opcode,"Else") == 0) {
                currIf->end = i;
                
                Comp c = {
                    .opcode = "",
                    .end = i+1,  // Default end to next line
                    .joinedIf = currIf
                };
                
                Pseudo p = {
                    .type = COMP,
                    .comp = c
                };
                
                pseudoInst[i] = p;
                currIf = &pseudoInst[i].comp;
            }
            else if(strcmp(opcode,"While") == 0) {
                loop = true;

                updateLifespan(operandList[0], vars, i, loop);
                updateLifespan(operandList[2], vars, i, loop);

                Comp c = {
                    .opcode = operandList[1],
                    .operand1 = operandList[0],
                    .operand2 = operandList[2],
                    .start = i,
                    .end = i+1,  // Default end to next line
                    .joinedIf = currIf
                };
                
                Pseudo p = {
                    .type = COMP,
                    .comp = c
                };
                
                pseudoInst[i] = p;
                currIf = &pseudoInst[i].comp;
            }
            else if(strcmp(opcode,"End") == 0) {
                currIf->end = i;
                End e = {.loop = NULL};
                if (loop) {
                    updateLoopVars(vars, i);
                    e.loop = currIf;
                    loop = false;
                }

                Pseudo p = {
                    .type = END,
                    .end = e
                };
                pseudoInst[i] = p;
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
                removeRegFromUsed(usedRegs, src, vars->count);
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

void loadMem(char** usedRegs, Vars* vars, Gadgets gadgets) {
    for (int i = 0 ; i < gadgets.numLoadMemGadgets ; i++) {
        Gadget loadGadget = gadgets.loadMemGadgets[i];
        char* dest = loadGadget.operands[0];
        char* srcAddr = loadGadget.operands[1];
        char* clearReg;
        
        // Only gadgets have srcAddr = dest for now .. TODO
        if (exists(srcAddr,usedRegs,vars->count)) {
            clearReg = moveRegAnywhere(srcAddr, usedRegs, vars, gadgets);
        }
        else {
            clearReg = "";
        }

        if (clearReg != NULL){
            char* assembly = malloc(strlen(loadGadget.assembly) + strlen(clearReg) + 2);
            strcat(assembly,clearReg);
            strcat(assembly,"\n");
            // strcat()
        }


        // Load const addr
        // loadMem

        // if (exists(dest,usedRegs,vars->count)) {
        //     moveRegAnywhere(dest, usedRegs, vars, gadgets);
        // } 
    }
}

char* loadConstValue(Var* var, char* dest, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;
    int count = vars->count;

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

char* checkRegisterPossible(Var* var, char* dest, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;
    int count = vars->count;
    // Already in correct register
    if(strcmp(var->reg,dest) == 0){
        return "";  // No change needed
    }
    else if(var->inMemory){
        ;
    }
    // Unloaded var
    else if(strcmp(var->reg,"new") == 0){
        return loadConstValue(var, dest, usedRegsPtr, varsPtr, gadgets);
    }
    // Loaded but in wrong register
    else {
        return moveReg(var, dest, usedRegs, vars, gadgets);
    }
    return NULL;
}

// Create type a=a+b
void synthesizeArith(ArithOp inst, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    int count = vars->count;
    char op = inst.opcode;

    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++){
        char** usedRegs = usedRegisters(vars);
        Vars* tmpVars = copyVars(vars);
        Var* a = findVar(inst.operand1,tmpVars);
        Var* b = findVar(inst.operand2,tmpVars);
        Gadget gadget = gadgets.arithOpGadgets[i];

        if ( (op == '+' &&  strcmp(gadget.opcode,"add") == 0) ||
             (op == '-' &&  strcmp(gadget.opcode,"sub") == 0)  ) {
            char* setupA = checkRegisterPossible(a, gadget.operands[0], &usedRegs, &tmpVars, gadgets); 
            // WARNING may have moved value from add dest to add src - could now be stuck when other moves where possible
            char* setupB = checkRegisterPossible(b, gadget.operands[1], &usedRegs, &tmpVars, gadgets);
            if (setupA != NULL && setupB != NULL){
                // Set new register - could be the same if unchanged
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
    printf("Failed to find\n");
}

void translatePseudo(int progLines, Vars* vars, Pseudo* pseudoInst, Gadgets gadgets){
    for (int i = 0 ; i < progLines ; i++){
        deleteStaleVars(i, vars);
        
        switch (pseudoInst[i].type){
            case LOAD_CONST: {
                LoadConst inst = pseudoInst[i].loadConst;
                findVar(inst.out,vars)->value = inst.value;
                break;
            }
            case ARITH_OP: {
                ArithOp inst = pseudoInst[i].arithOp;
                synthesizeArith(inst, &vars, gadgets);
                switch (inst.opcode) {
                    case '+':
                        findVar(inst.operand1,vars)->value += findVar(inst.operand2,vars)->value;
                        findVar(inst.operand1,vars)->constant = false;
                        break;
                    
                    case '-':
                        findVar(inst.operand1,vars)->value -= findVar(inst.operand2,vars)->value;
                        findVar(inst.operand1,vars)->constant = false;
                        break;
                }
                break;
            }
            case COMP: {
                Comp* inst = &pseudoInst[i].comp;

                Var* a = findVar(inst->operand1, vars);
                Var* b = findVar(inst->operand2, vars);
                inst->valid = false;  // result of if is assumed false
                bool skip = false;  // Can skip valid else ifs
                
                if(inst->joinedIf != NULL && inst->joinedIf->valid) {
                    inst-> valid = true;  // Propagate validity to also skip chained elses
                    skip = true;  // Skip since previous if was true
                } 
                else if(strcmp(inst->opcode, "==") == 0){
                    if(a->value == b->value) inst->valid  = true;
                }
                else if(strcmp(inst->opcode, "<") == 0){
                    if(a->value < b->value) inst->valid  = true;
                }
                else if(strcmp(inst->opcode, ">") == 0){
                    if(a->value > b->value) inst->valid  = true;
                }
                else if(strcmp(inst->opcode, "<=") == 0){
                    if(a->value <= b->value) inst->valid  = true;
                }
                else if(strcmp(inst->opcode, ">=") == 0){
                    if(a->value >= b->value) inst->valid  = true;
                }
                else if(strcmp(inst->opcode, "") == 0){
                    inst->valid  = true;  // Else
                }
                else {
                    printf("Unknown operation\n");
                }


                if(!inst->valid || skip){
                    i = inst->end - 1;  // Skip to end of loop (sub one because of loop inc)
                }

                break;
            }
            case END: {
                End inst = pseudoInst[i].end;

                if (inst.loop != NULL && inst.loop->valid) {
                    i = inst.loop->start - 1;  // Go back to loop start
                }
                break;
            }
            default:
                break;
        }
        // printf("%d %d %d\n",findVar("x",vars)->value,findVar("y",vars)->value,findVar("z",vars)->value);
    }
}

int main(){
    // const int progLines = 11;
    // char* prog[progLines] = {
    //     "Var x 1",
    //     "Var y 2",
    //     "Add x y",
    //     "Var z 3",

    //     "If x > z",
    //         "Sub x z",
    //     "ElseIf x < z",
    //         "Add x y",
    //     "Else",
    //         "Add x z",
    //     "End"
    // };
    const int progLines = 6;
    char* prog[progLines] = {
        "Var x 3",
        "Var y 1",
        "Var z 0",

        "While x > z",
            "Sub x y",
        "End"
    };
    Vars *vars = malloc(sizeof(Vars) + sizeof(Var*)*progLines);
    vars->count = 0;
    Pseudo pseudoInst[progLines];
    
    createPseudo(progLines, prog, vars, pseudoInst);
    Gadgets gadgets = loadGadgets();
    translatePseudo(progLines, vars, pseudoInst, gadgets);
    
    return 0;
}