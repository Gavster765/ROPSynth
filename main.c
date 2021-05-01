#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "gadgets.h"
#include "utils.h"
#include "pseudo.h"
#include "var.h"
#include "synth-loop-free-prog/synthesis.h"

void translatePseudo(int progLines, Vars* *varsPtr, Pseudo* pseudoInst, Gadgets gadgets);
char* loadConstValue(Var* var, char* dest, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets);
char* storeMem(Var* var, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets);

// Read user program and parse into pseudo instructions
void createPseudo(int progLines, char** prog, Vars* vars, Pseudo* pseudoInst) {
    Comp* *currIf = malloc(sizeof(Comp*)*20);

    int currIfNum = -1; 
    bool loop = false;

    for (int i = 0 ; i < progLines ; i++){
            char* line = strdup(prog[i]);
            char* opcode = strtok(line, " ");
            char* operands = strtok(NULL, "");  // Save all operands
            char** operandList = calloc(6,20);  // Max 6 operands at 20 chars each
            getOperands(operandList, operands);
            if(strcmp(opcode,"Const") == 0){
                Var* newVar = addVar(operandList[0], vars);
                newVar->lifeSpan = i+1;
                newVar->value = atoi(operandList[1]);
                LoadConst newConst = {
                    .out = operandList[0],
                    .value = atoi(operandList[1]),
                    .instLoad = false
                };
                Pseudo p = {
                    .type = LOAD_CONST,
                    .loadConst = newConst
                };
                pseudoInst[i] = p;
            }
            else if(strcmp(opcode,"Var") == 0){
                Var* newVar = addVar(operandList[0], vars);
                newVar->lifeSpan = i+1;
                newVar->constant = false;
                LoadConst newConst = {
                    .out = operandList[0],
                    .value = atoi(operandList[1]),
                    .instLoad = false
                };
                Pseudo p = {
                    .type = LOAD_CONST,
                    .loadConst = newConst
                };
                pseudoInst[i] = p;
            }
            else if(strcmp(opcode,"Load") == 0){
                Var* newVar = addVar(operandList[0], vars);
                newVar->lifeSpan = i+1;
                newVar->constant = true;
                newVar->noKill = true;
                strcpy(newVar->reg,operandList[2]);
                Var* copyVar = findVar(operandList[1],vars);
                if (!copyVar->constant) {
                    newVar->memAddress = copyVar->memAddress;
                }
                LoadConst newConst = {
                    .out = operandList[0],
                    .value = copyVar->value,
                    .instLoad = true
                };
                Pseudo p = {
                    .type = LOAD_CONST,
                    .loadConst = newConst
                };
                pseudoInst[i] = p;
            }
            else if(checkArithOp(opcode)) {
                ArithOp newArith = {
                    .out = operandList[0],
                    .operand1 = operandList[0],
                    .operand2 = operandList[1]
                };

                fillArithOp(&newArith, opcode);

                Pseudo p = {
                    .type = ARITH_OP,
                    .arithOp = newArith
                };
                pseudoInst[i] = p;
                updateLifespan(operandList[0], vars, i, loop);
                updateLifespan(operandList[1], vars, i, loop);
            }
            else if(strcmp(opcode,"Copy") == 0) {
                Copy c = {
                    .dest = operandList[0],
                    .src = operandList[1]
                };

                Pseudo p = {
                    .type = COPY,
                    .copy = c
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
                    .loop = false,
                    .end = i+1,  // Default end to next line
                    .joinedIf = NULL,
                    .and = NULL
                };

                if (operandList[3] != NULL && strcmp(operandList[3],"And") == 0) {
                    Comp* and = malloc(sizeof(Comp));
                    and->opcode = operandList[5];
                    and->operand1 = operandList[4];
                    and->operand2 = operandList[6];
                    and->loop = true;
                    and->start = i;
                    and->end = i+1;  // Default end to next line
                    and->joinedIf = NULL;
                    and->and = NULL;
                    c.and = and;
                }

                Pseudo p = {
                    .type = COMP,
                    .comp = c
                };
                pseudoInst[i] = p;
                currIfNum++;
                currIf[currIfNum] = &pseudoInst[i].comp;
            }
            else if(strcmp(opcode,"ElseIf") == 0) {
                updateLifespan(operandList[0], vars, i, loop);
                updateLifespan(operandList[2], vars, i, loop);
                currIf[currIfNum]->end = i;
                
                Comp c = {
                    .opcode = operandList[1],
                    .operand1 = operandList[0],
                    .operand2 = operandList[2],
                    .loop = false,
                    .end = i+1,  // Default end to next line
                    .joinedIf = currIf[currIfNum],
                    .and = NULL
                };

                if (operandList[3] != NULL && strcmp(operandList[3],"And") == 0) {
                    Comp* and = malloc(sizeof(Comp));
                    and->opcode = operandList[5];
                    and->operand1 = operandList[4];
                    and->operand2 = operandList[6];
                    and->loop = true;
                    and->start = i;
                    and->end = i+1;  // Default end to next line
                    and->joinedIf = NULL;
                    and->and = NULL;
                    c.and = and;
                }
                
                Pseudo p = {
                    .type = COMP,
                    .comp = c
                };
                
                pseudoInst[i] = p;
                currIf[currIfNum] = &pseudoInst[i].comp;
            }
            else if(strcmp(opcode,"Else") == 0) {
                currIf[currIfNum]->end = i;
                
                Comp c = {
                    .opcode = "",
                    .loop = false,
                    .end = i+1,  // Default end to next line
                    .joinedIf = currIf[currIfNum],
                    .and = NULL
                };
                
                Pseudo p = {
                    .type = COMP,
                    .comp = c
                };
                
                pseudoInst[i] = p;
                currIf[currIfNum] = &pseudoInst[i].comp;
            }
            else if(strcmp(opcode,"While") == 0) {
                loop = true;

                updateLifespan(operandList[0], vars, i, loop);
                updateLifespan(operandList[2], vars, i, loop);

                Comp c = {
                    .opcode = operandList[1],
                    .operand1 = operandList[0],
                    .operand2 = operandList[2],
                    .loop = true,
                    .start = i,
                    .end = i+1,  // Default end to next line
                    .joinedIf = NULL,
                    .and = NULL
                };
                
                if (operandList[3] != NULL && strcmp(operandList[3],"And") == 0) {
                    Comp* and = malloc(sizeof(Comp));
                    and->opcode = operandList[5];
                    and->operand1 = operandList[4];
                    and->operand2 = operandList[6];
                    and->loop = true;
                    and->start = i;
                    and->end = i+1;  // Default end to next line
                    and->joinedIf = NULL;
                    and->and = NULL;
                    c.and = and;
                }

                Pseudo p = {
                    .type = COMP,
                    .comp = c
                };
                
                pseudoInst[i] = p;
                currIfNum++;
                currIf[currIfNum] = &pseudoInst[i].comp;
            }
            else if(strcmp(opcode,"End") == 0) {
                Comp* curr = currIf[currIfNum];
                curr->end = i;
                if (curr->and != NULL) {
                    curr->and->finish = i;
                    curr->and->end = i;
                }
                // Set the finish point for all ifs in chain
                Comp* prev = curr->joinedIf;
                while (prev != NULL) {
                    if (prev->and != NULL) {
                        prev->and->finish = i;
                        prev->and->end = prev->end;
                    }
                    prev->finish = i;
                    prev = prev->joinedIf;
                }

                End e = {.loop = NULL};
                if (curr->loop) {
                    curr->finish = i;
                    updateLoopVars(vars, i);
                    e.loop = curr;
                    loop = false;
                }

                Pseudo p = {
                    .type = END,
                    .end = e
                };
                pseudoInst[i] = p;
                currIfNum--;
            }
            else if(strcmp(opcode, "Jump") == 0) {
                updateLifespan(operandList[1], vars, i, loop);
                updateLifespan(operandList[3], vars, i, loop);
                
                Jump j = {
                    .dest = atoi(operandList[0]),
                    .breakDest = NULL,
                    .opcode = operandList[2],
                    .operand1 = operandList[1],
                    .operand2 = operandList[3]
                };

                Pseudo p = {
                    .type = JUMP,
                    .jump = j
                };
                pseudoInst[i] = p;
            }
            else if(strcmp(opcode, "Break") == 0) {                
                Jump j = {
                    .dest = -1,
                    .opcode = "_"
                };

                for (int c = currIfNum ; c >= 0 ; c--) {
                    if (currIf[c]->loop) {
                        j.breakDest = &currIf[c]->finish;
                        break;
                    }
                }

                Pseudo p = {
                    .type = JUMP,
                    .jump = j
                };
                pseudoInst[i] = p;
            }
            else if(checkSpecialOp(opcode)) {
                Special s = {
                    .operand = operandList[0]
                };
                
                fillSpecialOp(&s, opcode);
                
                Pseudo p = {
                    .type = SPECIAL,
                    .special = s
                };

                pseudoInst[i] = p;
                updateLifespan(operandList[0], vars, i, loop);
            }
            free(operandList);
            free(line);
        }
    free(currIf);
}

// Attempt to free src by moving the store var anywhere it can
char* moveRegAnywhere(char* src, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets) {
    char* assembly = storeMem(findVarByReg(src, *varsPtr),usedRegsPtr,varsPtr,gadgets);
    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;

    if (assembly != NULL){
        return assembly;
    }

    for (int i = 0 ; i < gadgets.numMoveRegGadgets ; i++) {
        Gadget moveGadget = gadgets.moveRegGadgets[i];
        if (strcmp(src, moveGadget.operands[1]) == 0 &&
            !used(moveGadget.operands[0], usedRegs, vars->count)) {
                Var* var = findVarByReg(src, vars);
                strcpy(var->reg, moveGadget.operands[0]);
                removeRegFromUsed(usedRegs, src, vars->count);
                addRegToUsed(usedRegs, var->reg, vars->count);
                return strdup(moveGadget.assembly);
        }
    }
    return NULL;
}

// Try to move var to dest by mean of moves - can also move a var out of dest if required
char* moveReg(Var* var, char* dest, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets){
    if (strcmp(var->reg, dest) == 0){
        return strdup("");
    }
    char* varName = strdup(var->name);
    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;
    char* assembly;
    if (used(dest, usedRegs, vars->count)){
        char* moveExisting = moveRegAnywhere(dest, usedRegsPtr, varsPtr, gadgets);
        vars = *varsPtr;
        var = findVar(varName, vars);
        usedRegs = *usedRegsPtr;
        if (moveExisting == NULL){
            free(varName);
            return NULL;
        }
        assembly = malloc(strlen(gadgets.moveRegGadgets[0].assembly) +
                          strlen(moveExisting) + sizeof(int) + 2);
        assembly[0] = '\0';    
        strcat(assembly,moveExisting);
        strcat(assembly,"\n");
        free(moveExisting);
    }
    else {
        assembly = malloc(2 * strlen(gadgets.moveRegGadgets[0].assembly) + sizeof(int) + 2);
        assembly[0] = '\0';    
    }
    if (strcmp(var->reg, "new") == 0) {
        char* load = loadConstValue(var, dest, usedRegsPtr, varsPtr, gadgets);
        usedRegs = *usedRegsPtr;
        strcat(assembly,load);
        free(varName);
        free(load);
        return assembly;
    }
    for (int i = 0 ; i < gadgets.numMoveRegGadgets ; i++) {
        Gadget moveGadget = gadgets.moveRegGadgets[i];
        if (strcmp(dest, moveGadget.operands[0]) == 0 &&
            strcmp(var->reg, moveGadget.operands[1]) == 0) {
                removeRegFromUsed(usedRegs, var->reg, vars->count);
                strcpy(var->reg,dest);
                strcat(assembly, moveGadget.assembly);
                addRegToUsed(usedRegs, dest, vars->count);
                free(varName);
                return assembly;
        }
    }
    free(assembly);
    free(varName);
    return NULL;
}

// Will  try to load a const from the stack into dest - can move other var out of dest
char* loadConstValue(Var* var, char* dest, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets){
    char* varName = strdup(var->name);
    char* moveAway;
    if (used(dest,*usedRegsPtr,(*varsPtr)->count)) {
        moveAway = moveRegAnywhere(dest, usedRegsPtr, varsPtr, gadgets);  // Reg in use - move first
        if (moveAway == NULL) {
            free(varName);
            return NULL;  // Can't free dest so fail
        }
    }
    else {
        moveAway = strdup("");
    }

    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;
    int count = vars->count;
    var = findVar(varName,vars);
    free(varName);

    // Check for direct load first
    for (int i = 0 ; i < gadgets.numLoadConstGadgets ; i++) {
        Gadget loadGadget = gadgets.loadConstGadgets[i];
        if (strcmp(loadGadget.operands[0],dest) == 0){
            strcpy(var->reg, dest);
            addRegToUsed(usedRegs, dest, vars->count);
            char* assembly = malloc(strlen(moveAway) + strlen(loadGadget.assembly) + sizeof(int) + 6);
            sprintf(assembly, "%s\n%s (%d)",moveAway,loadGadget.assembly,var->value);
            free(moveAway);
            return assembly;
        }
    }
    for (int i = 0 ; i < gadgets.numLoadConstGadgets ; i++) {
        Gadget loadGadget = gadgets.loadConstGadgets[i];
        char* gadgetDest = loadGadget.operands[0];
        if (!used(gadgetDest,usedRegs,count)){
            Vars* tmpVars = copyVars(vars);
            Var* tmpVar = findVar(var->name, tmpVars);
            strcpy(tmpVar->reg,gadgetDest);  // Temporially load
            char** tmpUsedRegs = usedRegisters(tmpVars);
            addRegToUsed(tmpUsedRegs, gadgetDest, tmpVars->count);
            char* possMove = moveReg(tmpVar, dest, &tmpUsedRegs, &tmpVars, gadgets);
            if (possMove != NULL){
                *varsPtr = tmpVars;
                *usedRegsPtr = tmpUsedRegs;
                int len = strlen(moveAway) + strlen(loadGadget.assembly) + strlen(possMove) + sizeof(int) + 6;
                char* assembly = malloc(len);
                assembly[0] = '\0';
                snprintf(assembly, len, "%s\n%s (%d)\n%s", moveAway, loadGadget.assembly, tmpVar->value, possMove);
                freeVars(vars);
                freeUsedRegs(usedRegs, count);
                free(possMove);
                free(moveAway);
                return assembly;
            }
            freeVars(tmpVars);
            freeUsedRegs(tmpUsedRegs, count);
        }
    }
    free(moveAway);
    return NULL;
}

// Store a var in memory so doesn't need to be in a reg
char* storeMem(Var* var, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets) {
    char* varName = strdup(var->name);
    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;
    int count = vars->count;

    for (int i = 0 ; i < gadgets.numStoreMemGadgets ; i++) {
        Gadget storeGadget = gadgets.storeMemGadgets[i];
        char* storeAddr = storeGadget.operands[0];
        char* storeData = storeGadget.operands[1];

        char* clearReg;
        char* loadAddr;
        char* moveData = moveReg(var, storeData, usedRegsPtr, varsPtr, gadgets);
        vars = *varsPtr;
        usedRegs = *usedRegsPtr;
        var = findVar(varName, vars);
        if (used(storeAddr,usedRegs,vars->count)) {
            printf("Warning!\n");
            continue;
            // clearReg = moveRegAnywhere(storeAddr, usedRegsPtr, varsPtr, gadgets);
        }
        else {
            clearReg = "";
        }

        if (clearReg != NULL) {
            Var* addressVar = vars->vars[0];
            addressVar->value = var->memAddress;
            loadAddr = loadConstValue(addressVar, storeAddr, usedRegsPtr, varsPtr, gadgets);
            if (loadAddr == NULL) {
                continue;
            }
            usedRegs = *usedRegsPtr;
            vars = *varsPtr;
            strcpy(vars->vars[0]->reg, "new");
        }
        // char* moveData = moveReg(findVar(varName, vars), storeData, usedRegsPtr, varsPtr, gadgets);
        usedRegs = *usedRegsPtr;
        if (moveData != NULL){
            int len = strlen(storeGadget.assembly) + strlen(clearReg) + strlen(moveData) +
                    strlen(loadAddr) + 4;
            char* assembly = malloc(len);
            snprintf(assembly, len, "%s\n%s\n%s\n%s",moveData,clearReg,loadAddr,
                    storeGadget.assembly);
            Var* v = findVar(varName, vars);
            v->inMemory = true;
            strcpy(v->reg,"new");

            removeRegFromUsed(usedRegs,storeData,count);
            removeRegFromUsed(usedRegs,storeAddr,count);
            free(varName);
            free(moveData);
            free(loadAddr);
            return assembly;
        }
        free(moveData);
        free(loadAddr);
    }
    free(varName);
    return NULL;
}

// Load a var from memory into a given reg for use - noMove is incase both operands need to be loaded using the same load gadget
char* loadMem(Var* var, char* dest, Var* noMove, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets) {
    char* varName = strdup(var->name);
    char* noMoveVarName;
    char noMoveVarReg[4];
    if (noMove != NULL) {
        noMoveVarName = strdup(noMove->name);
        strcpy(noMoveVarReg,noMove->reg);
        if (strcmp(dest,noMoveVarReg) == 0) {
            // Impossible since both vars need to be in same reg
            free(varName);
            free(noMoveVarName);
            return NULL;
        }
    }

    for (int i = 0 ; i < gadgets.numLoadMemGadgets ; i++) {
        Gadget loadGadget = gadgets.loadMemGadgets[i];
        char* loadDest = loadGadget.operands[0];
        char* srcAddr = loadGadget.operands[1];
        char* clearReg = NULL;
        char* clearRegAddr = NULL;
        char* loadAddr = NULL;
        char* move;
        char* moveBack;
        Vars* tmpVars = copyVars(*varsPtr);

        if (used(loadDest,*usedRegsPtr,tmpVars->count)) {
            clearReg = moveRegAnywhere(loadDest, usedRegsPtr, &tmpVars, gadgets);
        }
        else {
            clearReg = strdup("");
        }

        if (used(srcAddr,*usedRegsPtr,tmpVars->count)) {
            clearRegAddr = moveRegAnywhere(srcAddr, usedRegsPtr, &tmpVars, gadgets);
        }
        else {
            clearRegAddr = strdup("");
        }

        if (clearReg != NULL && clearRegAddr != NULL) {
            Var* v = findVar(varName,tmpVars);
            int value = v->value;
            v->value = v->memAddress;
            loadAddr = loadConstValue(v, srcAddr, usedRegsPtr, &tmpVars, gadgets);
            v = findVar(varName, tmpVars);
            v->value = value;
        }

        if (loadAddr != NULL) {
            var = findVar(varName, tmpVars);
            strcpy(var->reg, loadDest);
            if (strcmp(dest,"any") == 0){
                move = strdup("");
            } 
            else {
                move = moveReg(var, dest, usedRegsPtr, &tmpVars, gadgets);
            }
        }
        
        if (noMove != NULL &&  move != NULL) {
            Var* noMoveVar = findVar(noMoveVarName, tmpVars);
            var = findVar(varName, tmpVars);
            if (strcmp(noMoveVarReg, noMoveVar->reg) != 0){
                // TODO - combine with prev move? so can make move safe
                if (noMoveVar->inMemory) {
                    // Possible infinite recursion - consider NULL for var with clobber check?
                    moveBack = loadMem(noMoveVar, noMoveVarReg, var, usedRegsPtr, &tmpVars, gadgets);
                } else {
                    moveBack = moveReg(noMoveVar, noMoveVarReg, usedRegsPtr, &tmpVars, gadgets);
                }
            }
            else {
                moveBack = strdup("");
            }
            free(noMoveVarName);
        }
        else {
            moveBack = strdup("");
        }
        
        if (clearReg != NULL && clearRegAddr != NULL && loadAddr != NULL && move != NULL && moveBack != NULL){
            int len = strlen(loadGadget.assembly) + strlen(loadAddr) + strlen(clearReg) + 
                      strlen(clearRegAddr) + strlen(move) + strlen(moveBack) + 6;
            char* assembly = malloc(len);
            snprintf(assembly, len, "%s\n%s\n%s\n%s\n%s\n%s",clearReg,clearRegAddr,loadAddr,
                    loadGadget.assembly,move,moveBack);
            Vars* tmp = *varsPtr;
            *varsPtr = tmpVars;
            freeVars(tmp);
            free(clearReg);
            free(clearRegAddr);
            free(loadAddr);
            free(move);
            free(moveBack);
            free(varName);
            return assembly;
        }
        freeVars(tmpVars);
        free(clearReg);
        free(clearRegAddr);
        free(loadAddr);
        free(move);
        free(moveBack);
    }
    free(varName);
    if (noMove != NULL) {
        free(noMoveVarName);
    }
    return NULL;
}

// Attempt to move a var into dest for use
char* checkRegisterPossible(Var* var, char* dest, Var* noMove, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets){
    // Already in correct register
    if(strcmp(var->reg,dest) == 0){
        return strdup("");  // No change needed
    }
    else if(var->inMemory){
        return loadMem(var, dest, noMove, usedRegsPtr, varsPtr, gadgets);
    }
    // Unloaded var
    else if(strcmp(var->reg,"new") == 0){
        return loadConstValue(var, dest, usedRegsPtr, varsPtr, gadgets);
    }
    // Loaded but in wrong register
    else {
        return moveReg(var, dest, usedRegsPtr, varsPtr, gadgets);
    }
    return NULL;
}

// Make sure all non constants are in memory so location is known
void storeAllVar(Vars* *varsPtr, Gadgets gadgets) {
    int count = (*varsPtr)->count;
    for (int i = 0 ; i < count ; i++) {
        Var* v = (*varsPtr)->vars[i];
        if (!v->constant && !v->inMemory) {
            char** usedRegs = usedRegisters(*varsPtr);
            char* assembly = storeMem(v, &usedRegs, varsPtr, gadgets);
            if (assembly != NULL) {
                printf("%s\n",assembly);
                free(assembly);
            }
            else {
                printf("Warning could not store\n");
            }
            freeUsedRegs(usedRegs, count);
        }
    }
}

char* synthesizeCopy(Copy inst, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    Var* dest = findVar(inst.dest, vars);
    int number;
    
    // Copy number
    if (inst.src[0] == '0' || sscanf(inst.src,"%d", &number) == 1) {
        dest->value = (int)strtol(inst.src, NULL, 0);
        strcpy(dest->reg, "new");
        dest->constant = true;
        dest->inMemory = false;
        return strdup("");
    }

    Var* src = findVar(inst.src, vars);
    // If src is constant just update value as if fresh
    if (src->constant) {
        dest->value = src->value;
        strcpy(dest->reg, "new");
        dest->constant = true;
        dest->inMemory = false;
        return strdup("");
    }
    // Case in memory
    else if (src->inMemory) {
        char** usedRegs = usedRegisters(vars);
        // char** *usedRegsPtr = &usedRegs;
        char* assembly = loadMem(src, "any", NULL, &usedRegs, varsPtr, gadgets);
        vars = *varsPtr;
        dest = findVar(inst.dest, vars);
        src = findVar(inst.src, vars);
        // Set properties of dest
        dest->value = src->value;
        strcpy(dest->reg, src->reg);
        dest->inMemory = false;
        dest->constant = false;
        // Reset loaded src
        strcpy(src->reg, "new");
        src->inMemory = true;
        freeUsedRegs(usedRegs, vars->count);
        return assembly;
    }
    // Case in reg
    else {
        // TODO check for NULL - i.e. impossible
        char** usedRegs = usedRegisters(vars);
        dest->value = src->value;
        dest->constant = false;
        dest->inMemory = false;
        strcpy(dest->reg, src->reg);
        char* assembly = moveRegAnywhere(src->reg, &usedRegs, varsPtr, gadgets);
        freeUsedRegs(usedRegs, (*varsPtr)->count);
        return assembly;
    }
}

// Create type a=a+b, return whether needs updating (yes=1,no=0,fail=-1)
// Write asm for arithmetic operations
int synthesizeArith(ArithOp inst, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    int count = vars->count;
    char op = inst.opcode;

    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++){
        char** usedRegs = usedRegisters(vars);
        Vars* tmpVars = copyVars(vars);
        Var* a = findVar(inst.operand1,tmpVars);
        Var* b = findVar(inst.operand2,tmpVars);
        Gadget gadget = gadgets.arithOpGadgets[i];

        if (checkArithOpGadget(op, gadget.opcode)) {
            char* setupA = checkRegisterPossible(a, gadget.operands[0], NULL, &usedRegs, &tmpVars, gadgets); 
            a = findVar(inst.operand1,tmpVars);
            b = findVar(inst.operand2,tmpVars);
            // WARNING may have moved value from add dest to add src - could now be stuck when other moves where possible
            char* setupB = checkRegisterPossible(b, gadget.operands[1], a, &usedRegs, &tmpVars, gadgets);
            if (setupA != NULL && setupB != NULL){
                // Set new register - could be the same if unchanged
                *varsPtr = tmpVars;
                freeVars(vars);
                freeUsedRegs(usedRegs, count);
                printf("%s\n%s\n%s\n",setupA,setupB,gadget.assembly);
                free(setupA);
                free(setupB);
                return true;
            }
            free(setupA);
            free(setupB);
        }
        freeUsedRegs(usedRegs, count);
        freeVars(tmpVars);
    }
    // Couldn't find gadget so try to find alternative
    Vars* tmpVars = copyVars(vars);
    char* alt = findAlternative(inst, tmpVars, gadgets);
    if (alt == NULL) {
        freeVars(tmpVars);
        return -1;  // Synthesis failed
    }

    int lines = 0;
    for (int i = 0 ; i < strlen(alt) ; i++) {
        if (alt[i] == '\n'){
            lines++;
        }
    }

    // All non fresh vars should have lifespans longer than alt prog
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            tmpVars->vars[i]->lifeSpan = lines + 1;
        }
    }
    char* altProg[lines];
    getProgLines(altProg, alt);
    Pseudo altPseudoInst[lines];
    createPseudo(lines, altProg, tmpVars, altPseudoInst);
    translatePseudo(lines, &tmpVars, altPseudoInst, gadgets);
    // Swap variables
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            Var* temp = vars->vars[i];
            (*varsPtr)->vars[i] = tmpVars->vars[i];
            (*varsPtr)->vars[i]->lifeSpan = temp->lifeSpan;  // Reset lifespan
            tmpVars->vars[i] = temp;
        }
    }
    free(alt);
    freeVars(tmpVars);
    freePseudo(lines, altPseudoInst);
    return false;
}

void synthesizeJump(Jump inst, Vars* vars, Gadgets gadgets) {
    Vars* tmpVars = copyVars(vars);
    char* progString = malloc(200);  // TODO - actual size
    int lines;
    progString[0] = '\0';

    Var* rspVar = addVar("_rsp", tmpVars);
    strcpy(rspVar->reg, "rsp");
    rspVar->constant = false;
    if (strcmp(inst.opcode, "<") == 0) {
        lines = 10;
        sprintf(progString,
            "Const _0 0\n"
            "Const _1 0\n"
            "Const _2 %d\n"
            "Copy _0 %s\n"
            "Sub _0 %s\n"
            "Adc _1 _1\n"
            "Neg _1\n"
            "Not _1\n"
            "And _1 _2\n"
            "Add _rsp _1\n",
            inst.dest,inst.operand1,inst.operand2
        );
    }
    else if (strcmp(inst.opcode, "<=") == 0) {
        lines = 9;
        sprintf(progString,
            "Const _0 0\n"
            "Const _1 0\n"
            "Const _2 %d\n"
            "Copy _0 %s\n"
            "Sub _0 %s\n"
            "Adc _1 _1\n"
            "Neg _1\n"
            "And _1 _2\n"
            "Add _rsp _1\n",
            inst.dest,inst.operand2,inst.operand1
        );
    }
    else if (strcmp(inst.opcode, ">") == 0) {
        lines = 10;
        sprintf(progString,
            "Const _0 0\n"
            "Const _1 0\n"
            "Const _2 %d\n"
            "Copy _0 %s\n"
            "Sub _0 %s\n"
            "Adc _1 _1\n"
            "Neg _1\n"
            "Not _1\n"
            "And _1 _2\n"
            "Add _rsp _1\n",
            inst.dest,inst.operand2,inst.operand1
        );
    }
    else if (strcmp(inst.opcode, ">=") == 0) {
        lines = 9;
        sprintf(progString,
            "Const _0 0\n"
            "Const _1 0\n"
            "Const _2 %d\n"
            "Copy _0 %s\n"
            "Sub _0 %s\n"
            "Adc _1 _1\n"
            "Neg _1\n"
            "And _1 _2\n"
            "Add _rsp _1\n",
            inst.dest,inst.operand1,inst.operand2
        );
    }
    else if (strcmp(inst.opcode, "=") == 0) {
        lines = 10;
        sprintf(progString,
            "Const _0 0\n"
            "Const _1 0\n"
            "Const _2 %d\n"
            "Copy _0 %s\n"
            "Sub _0 %s\n"
            "Neg _0\n"
            "Adc _1 _1\n"
            "Neg _1\n"
            "And _1 _2\n"
            "Add _rsp _1\n",
            inst.dest,inst.operand1,inst.operand2
        );
    }
    else if (strcmp(inst.opcode, "!=") == 0) {
        lines = 10;
        sprintf(progString,
            "Const _0 0\n"
            "Const _1 0\n"
            "Const _2 %d\n"
            "Copy _0 %s\n"
            "Sub _0 %s\n"
            "Neg _0\n"
            "Adc _1 _1\n"
            "Neg _1\n"
            "Not _1\n"
            "And _1 _2\n"
            "Add _rsp _1\n",
            inst.dest,inst.operand1,inst.operand2
        );
    }
    // Always jump
    else if (strcmp(inst.opcode, "_") == 0) {
        lines = 2;
        sprintf(progString,
        "Const _0 %d\n"
        "Add _rsp _0\n",
        inst.dest
        );
    }
    else {
        printf("Invalid jump\n");
        free(tmpVars);
        return;
    }

    // All non fresh vars should have lifespans longer than alt prog
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            tmpVars->vars[i]->lifeSpan = lines + 1;
        }
    }
    char* prog[lines];
    getProgLines(prog, progString);

    Pseudo progPseudoInst[lines];
    createPseudo(lines, prog, tmpVars, progPseudoInst);
    translatePseudo(lines, &tmpVars, progPseudoInst, gadgets);
    // Swap variables
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            Var* temp = vars->vars[i];
            vars->vars[i] = tmpVars->vars[i];
            vars->vars[i]->lifeSpan = temp->lifeSpan;  // Reset lifespan
            tmpVars->vars[i] = temp;
        }
    }
    free(progString);
    freeVars(tmpVars);
    freePseudo(lines, progPseudoInst);
}

void synthesizeSyscall(Special inst, Vars* *varsPtr, Gadgets gadgets) {
    char* syscallGadget = NULL;
    for (int i = 0 ; i < gadgets.numSpecialGadgets ; i++) {
        if (strcmp(gadgets.specialGadgets[i].opcode,"syscall") == 0) {
            syscallGadget = gadgets.specialGadgets[i].assembly;
            break;
        }
    }
    if (syscallGadget == NULL) {
        printf("No syscall gadget\n");
        return;
    }

    storeAllVar(varsPtr, gadgets);

    Vars* vars = *varsPtr;
    Vars* tmpVars = copyVars(vars);
    char* progString = malloc(200);  // TODO - actual size
    int lines;
    progString[0] = '\0';

    int bufferLocation = findVar(inst.operand, tmpVars)->memAddress;
    switch (inst.opcode) {
        case 'r': {
            lines = 7;
            sprintf(progString,
                "Const _0 0\n"
                "Var _1 8\n"
                "Const _2 %d\n"
                "Load _rax _0 rax\n"
                "Load _rdi _0 rdi\n"
                "Load _rsi _2 rsi\n"
                "Load _rdx _1 rdx\n",
                bufferLocation
            );
            break;
        }
        case 'w': {
            lines = 7;
            sprintf(progString,
                "Const _0 1\n"
                "Var _1 8\n"
                "Const _2 %d\n"
                "Load _rax _0 rax\n"
                "Load _rdi _0 rdi\n"
                "Load _rsi _2 rsi\n"
                "Load _rdx _1 rdx\n",
                bufferLocation
            );
            break;
        }
        default: {
            printf("Invalid syscall\n");
            freeVars(tmpVars);
            free(progString);
            return;
        }
    }

    // All non fresh vars should have lifespans longer than alt prog
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            tmpVars->vars[i]->lifeSpan = lines + 1;
        }
    }
    char* prog[lines];
    getProgLines(prog, progString);

    Pseudo progPseudoInst[lines];
    createPseudo(lines, prog, tmpVars, progPseudoInst);
    translatePseudo(lines, &tmpVars, progPseudoInst, gadgets);
    // Swap variables
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            Var* temp = vars->vars[i];
            vars->vars[i] = tmpVars->vars[i];
            vars->vars[i]->lifeSpan = temp->lifeSpan;  // Reset lifespan
            tmpVars->vars[i] = temp;
        }
    }

    printf("%s\n",syscallGadget);

    free(progString);
    freeVars(tmpVars);
    freePseudo(lines, progPseudoInst);
}

void synthesizeSyscallOld(Special inst, Vars* *varsPtr, Gadgets gadgets) {
    storeAllVar(varsPtr, gadgets); // Ensure all regs free

    printf("%s %c %s\n",inst.op,inst.opcode,inst.operand);
    char** usedRegs = usedRegisters(*varsPtr);
    Var* tmpVar = (*varsPtr)->vars[0];
    tmpVar->value = 0;
    char* loadOp = loadConstValue(tmpVar, "rax", &usedRegs, varsPtr, gadgets);
    char* loadFd = loadConstValue(tmpVar, "rdi", &usedRegs, varsPtr, gadgets);
    tmpVar->value = findVar(inst.operand, *varsPtr)->memAddress;
    char* loadBuf = loadConstValue(tmpVar, "rsi", &usedRegs, varsPtr, gadgets);
    tmpVar->value = 1;
    char* storeTmp =  storeMem(tmpVar, &usedRegs, varsPtr, gadgets);
    tmpVar = (*varsPtr)->vars[0];

    char* loadCount = loadConstValue(tmpVar, "rdx", &usedRegs, varsPtr, gadgets);
    printf("%s %s %s %s %s\n",loadOp,loadFd,loadBuf,storeTmp,loadCount);
    
    freeUsedRegs(usedRegs,(*varsPtr)->count);
    free(loadOp);
    free(loadFd);
    free(loadBuf);
    free(loadCount);
}

void synthesizeSpecial(Special inst, Vars* *varsPtr, Gadgets gadgets) {
    Vars* vars = *varsPtr;
    for (int i = 0 ; i < gadgets.numSpecialGadgets ; i++) {
        Gadget gadget = gadgets.specialGadgets[i];
        Vars* tmpVars = copyVars(vars);
        char** usedRegs = usedRegisters(tmpVars);
        if (checkSpecialOpGadget(inst.opcode, gadget.opcode)) {
            Var* a = findVar(inst.operand, tmpVars);
            char* setup = checkRegisterPossible(a, gadget.operands[0], NULL, &usedRegs, &tmpVars, gadgets);
            if (setup != NULL) {   
                *varsPtr = tmpVars;
                freeVars(vars);
                freeUsedRegs(usedRegs, (*varsPtr)->count);
                printf("%s\n%s\n",setup,gadget.assembly);
                free(setup);
                return;
            }
        }
        freeUsedRegs(usedRegs, tmpVars->count);
        freeVars(tmpVars);
    }
    // TODO - call cegis on fail - e.g. for not
}

// Read list of pseudo instructions and write required asm
void translatePseudo(int progLines, Vars* *varsPtr, Pseudo* pseudoInst, Gadgets gadgets){
    for (int i = 0 ; i < progLines ; i++){
        Vars* vars = *varsPtr;
        deleteStaleVars(i, vars);
        switch (pseudoInst[i].type){
            case LOAD_CONST: {
                LoadConst inst = pseudoInst[i].loadConst;
                Var* v = findVar(inst.out,vars);
                v->value = inst.value;
                if (!v->constant) {
                    char** usedRegs = usedRegisters(vars);
                    char* res = storeMem(v, &usedRegs, varsPtr, gadgets);
                    freeUsedRegs(usedRegs, (*varsPtr)->count);
                    printf("%s\n",res);
                    free(res);
                }
                else if (inst.instLoad) {
                    char** usedRegs = usedRegisters(vars);
                    // removeRegFromUsed(usedRegs, v->reg, (*varsPtr)->count);
                    char* dest = strdup(v->reg);
                    char* res = loadConstValue(v,dest,&usedRegs,varsPtr,gadgets);

                    if (res == NULL) {
                        res = loadMem(v, dest, NULL, &usedRegs, varsPtr, gadgets);
                    }

                    freeUsedRegs(usedRegs, (*varsPtr)->count);
                    free(dest);
                    printf("%s\n",res);
                    free(res);
                }
                break;
            }
            case ARITH_OP: {
                ArithOp inst = pseudoInst[i].arithOp;
                int update = synthesizeArith(inst, varsPtr, gadgets);
                vars = *varsPtr;
                if (update == -1) {
                    printf("Could not synthesize\n");
                    return;
                }
                else if (update == 1) {
                    findVar(inst.operand1,vars)->constant = false;
                    findVar(inst.operand1,vars)->inMemory = false;
                    switch (inst.opcode) {
                        case '+':
                            findVar(inst.operand1,vars)->value += findVar(inst.operand2,vars)->value;
                            break;
                        case '-':
                            findVar(inst.operand1,vars)->value -= findVar(inst.operand2,vars)->value;
                            break;
                        case '*':
                            findVar(inst.operand1,vars)->value *= findVar(inst.operand2,vars)->value;
                            break;
                        case '/':
                            findVar(inst.operand1,vars)->value *= findVar(inst.operand2,vars)->value;
                            break;
                        case '&':
                            findVar(inst.operand1,vars)->value &= findVar(inst.operand2,vars)->value;
                            break;
                        case '%':
                            findVar(inst.operand1,vars)->value %= findVar(inst.operand2,vars)->value;
                            break;
                    }
                }
                break;
            }
            case COPY: {
                Copy inst = pseudoInst[i].copy;
                char* copy = synthesizeCopy(inst, varsPtr, gadgets);                
                if (copy == NULL) {
                    printf("Can't copy\n");
                }
                printf("%s\n",copy);
                free(copy);
                break;
            }
            case COMP: {
                Comp* inst = &pseudoInst[i].comp;
                storeAllVar(varsPtr, gadgets);
                vars = *varsPtr;
                // Jump over elseif/else if required
                if (inst->joinedIf != NULL) {
                    Jump j = {
                        .dest = 2000 + inst->joinedIf->finish,
                        .opcode = "_"
                    };
                    synthesizeJump(j, vars, gadgets);
                } 
                // No jump for else
                if (strcmp(inst->opcode, "") != 0 ) {
                    Jump j = {
                        .dest = inst->end + 2000,
                        .opcode = inst->opcode,
                        .operand1 = inst->operand1,
                        .operand2 = inst->operand2
                    };
                    synthesizeJump(j, vars, gadgets);
                    if (inst->and != NULL) {
                        Jump j2 = {
                            .dest = inst->and->end + 2000,
                            .opcode = inst->and->opcode,
                            .operand1 = inst->and->operand1,
                            .operand2 = inst->and->operand2
                        };
                        synthesizeJump(j2, vars, gadgets);
                    }
                }
                break;
            }
            case END: {
                End inst = pseudoInst[i].end;
                storeAllVar(varsPtr, gadgets);

                // End of loop so jump back to start
                if (inst.loop != NULL) {
                    Jump j = {
                        .dest = -2000 - inst.loop->start,
                        .opcode = "_"
                    };
                    synthesizeJump(j, *varsPtr, gadgets);
                }
                break;
            }
            case JUMP: {
                Jump inst = pseudoInst[i].jump;
                if (inst.dest == -1) {
                    inst.dest = *inst.breakDest + 2000;
                }
                else {
                    inst.dest += 2000;
                }
                synthesizeJump(inst, vars, gadgets);
                break;
            }
            case SPECIAL: {
                Special inst = pseudoInst[i].special;
                switch (inst.opcode) {
                    case 'r': {
                        synthesizeSyscall(inst, varsPtr, gadgets);
                        break;
                    }
                    case 'w': {
                        synthesizeSyscall(inst, varsPtr, gadgets);
                        break;
                    }
                    default: {
                        synthesizeSpecial(inst, varsPtr, gadgets);
                    }
                }
                Var* v = findVar(inst.operand,*varsPtr);
                switch (inst.opcode) {
                    case '~': {
                        v->constant = false;
                        v->value = ~v->value;
                        v->value ++;
                        break;
                    }
                    case '!': {
                        v->constant = false;
                        v->value = ~v->value;
                        break;
                    }
                    case 'w': {
                        v->constant = false;
                        break;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

char** readProgram(char* fileName, int* length) {
    FILE *f = fopen(fileName, "r");

    // Get file length
    char ch;
    *length = 0;
    while(!feof(f)) {
        ch = fgetc(f);
        if(ch == '\n')
            {
                (*length)++;
            }
    }
    rewind(f);

    char** prog = malloc(*length * sizeof(char*));
    int max = 100;
    char* line = malloc(max);
    char* lineStart = line;
    int lineNum = 0;


    fgets(line, max, f);
    while (!feof(f)) {
        line[strcspn(line, "\r\n")] = '\0';
        removeLeadingSpaces(&line);
        // Skip blank lines
        if (strcmp(line, "") == 0) {
            (*length)--;
            line = lineStart;
            fgets(line, max, f);
            continue;
        }
        prog[lineNum] = malloc(strlen(line)+1);
        strcpy(prog[lineNum], line);
        lineNum++;
        line = lineStart;
        fgets(line, max, f);
    }
    free(line);
    fclose(f);
    return prog;
}

void freeProg(char** prog, int* length) {
    for (int i = 0 ; i < *length ; i++) {
        free(prog[i]);
    } 
    free(length);
    free(prog);
}

int main(int argc, char *argv[]) {
    if ( argc != 2 ) {
        printf("No program provided\n");
        return -1;
    }

    int* length = malloc(sizeof(int*));
    char** prog = readProgram(argv[1], length);
    int progLines = *length;

    // Allocate space for variables and pseudo instructions
    Vars *vars = malloc(sizeof(Vars) + sizeof(Var*)*(progLines+10));
    vars->count = 0;
    vars->maxSize = progLines+10;

    Var *addressVar = malloc(sizeof(Var) + 2);
    strcpy(addressVar->name, "");
    addressVar->lifeSpan = 0;
    addressVar->loop = false;
    addressVar->constant = true;
    addressVar->inMemory = false;
    addressVar->address = true;
    addressVar->noKill = false;

    vars->vars[0] = addressVar;
    vars->count = 1;
    Pseudo pseudoInst[progLines];
    
    // Parse program into pseudo instructions
    createPseudo(progLines, prog, vars, pseudoInst);
    // Read gadgets file
    Gadgets gadgets = loadGadgets();
    staticSynthesis(gadgets);
    translatePseudo(progLines, &vars, pseudoInst, gadgets);
    // printf("\n__Results__\n");
    // for (int i = 1 ; i < vars->count ; i ++) {
    //     Var* v = vars->vars[i];
    //     printf("%s: %d\n",v->name,v->value);
    // }
    freeProg(prog, length);
    freeVars(vars);
    freeGadgets(gadgets);
    freePseudo(progLines, pseudoInst);
    return 0;
}
