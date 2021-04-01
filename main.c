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
    Comp *currIf;  // Currently open if - TODO nested?
    bool loop = false;

    for (int i = 0 ; i < progLines ; i++){
            char* line = strdup(prog[i]);
            char* opcode = strtok(line, " ");
            char* operands = strtok(NULL, "");  // Save all operands
            char** operandList = malloc(3*20);  // Max 3 operands at 20 chars each
            getOperands(operandList, operands);
            if(strcmp(opcode,"Const") == 0){
                Var* newVar = addVar(operandList[0], vars);
                newVar->lifeSpan = i+1;
                LoadConst newConst = {
                    .out = operandList[0],
                    .value = atoi(operandList[1])
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
                    .value = atoi(operandList[1])
                };
                Pseudo p = {
                    .type = LOAD_CONST,
                    .loadConst = newConst
                };
                pseudoInst[i] = p;
            }
            // TODO move check if arith to func?
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
                    .joinedIf = NULL
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
                // Set the finish point for all ifs in chain
                Comp* prev = currIf->joinedIf;
                while (prev != NULL) {
                    prev->finish = i;
                    prev = prev->joinedIf;
                }

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
            else if(strcmp(opcode, "Jump") == 0) {
                updateLifespan(operandList[1], vars, i, loop);
                updateLifespan(operandList[3], vars, i, loop);
                
                Jump j = {
                    .dest = atoi(operandList[0]),
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
            // free(operandList);
            // free(line);
        }
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
                return moveGadget.assembly;
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
    // WARNING assumes no move gagdet longer than first and MEM LEAK?
    char* assembly;// = malloc(2 * strlen(gadgets.moveRegGadgets[0].assembly) + 2);
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

// TODO move then load?
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
        moveAway = "";
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
            sprintf(assembly, "%s\n%s (%d)",moveAway,loadGadget.assembly,var->value);  // TODO - free
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
                // WARNING memory leak!
                int len = strlen(moveAway) + strlen(loadGadget.assembly) + strlen(possMove) + sizeof(int) + 6;
                char* assembly = malloc(len);
                assembly[0] = '\0';
                snprintf(assembly, len, "%s\n%s (%d)\n%s", moveAway, loadGadget.assembly, tmpVar->value, possMove);
                freeVars(vars);
                freeUsedRegs(usedRegs, count);
                free(possMove);
                return assembly;
            }
            freeVars(tmpVars);
            freeUsedRegs(tmpUsedRegs, count);
        }
    }
    return NULL;  // Load not possible without move TODO
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
        // printf("store var: %s\n",varName);
        char* moveData = moveReg(findVar(varName, vars), storeData, usedRegsPtr, varsPtr, gadgets);
        usedRegs = *usedRegsPtr;
        if (moveData != NULL){
            int len = strlen(storeGadget.assembly) + strlen(clearReg) + strlen(moveData) +
                    strlen(loadAddr) + 4;
            char* assembly = malloc(len);
            snprintf(assembly, len, "%s\n%s\n%s\n%s",clearReg,loadAddr,moveData,
                    storeGadget.assembly);
            Var* v = findVar(varName, vars);
            v->inMemory = true;
            strcpy(v->reg,"new");

            removeRegFromUsed(usedRegs,storeData,count);
            removeRegFromUsed(usedRegs,storeAddr,count);
            // printf("store part: %s\n",assembly);
            // printf("done\n");
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
            return NULL;
        }
    }

    for (int i = 0 ; i < gadgets.numLoadMemGadgets ; i++) {
        Gadget loadGadget = gadgets.loadMemGadgets[i];
        char* loadDest = loadGadget.operands[0];
        char* srcAddr = loadGadget.operands[1];
        char* clearReg = NULL;
        char* loadAddr = NULL;
        char* move;
        char* moveBack;

        // Only gadgets have srcAddr = dest for now .. TODO
        if (used(srcAddr,*usedRegsPtr,(*varsPtr)->count)) {
            clearReg = moveRegAnywhere(srcAddr, usedRegsPtr, varsPtr, gadgets);
        }
        else {
            clearReg = strdup("");
        }

        if (clearReg != NULL) {
            Var* v = findVar(varName,*varsPtr);
            int value = v->value;
            v->value = v->memAddress;
            loadAddr = loadConstValue(v, srcAddr, usedRegsPtr, varsPtr, gadgets);
            v = findVar(varName,*varsPtr);
            v->value = value;
        }

        if (loadAddr != NULL) {
            var = findVar(varName, *varsPtr);
            strcpy(var->reg, loadDest);
            if (strcmp(dest,"any") == 0){
                move = strdup("");
            } 
            else {
                move = moveReg(var, dest, usedRegsPtr, varsPtr, gadgets);
            }
        }
        
        if (noMove != NULL) {
            Var* noMoveVar = findVar(noMoveVarName, *varsPtr);
            var = findVar(varName, *varsPtr);
            if (strcmp(noMoveVarReg, noMoveVar->reg) != 0){
                // TODO - combine with prev move? so can make move safe
                if (noMoveVar->inMemory) {
                    // Possible infinite recursion - consider NULL for var with clobber check?
                    moveBack = loadMem(noMoveVar, noMoveVarReg, var, usedRegsPtr, varsPtr, gadgets);
                } else {
                    moveBack = moveReg(noMoveVar, noMoveVarReg, usedRegsPtr, varsPtr, gadgets);
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
        
        if (clearReg != NULL && loadAddr != NULL && move != NULL && moveBack != NULL){
            int len = strlen(loadGadget.assembly) + strlen(loadAddr) + strlen(clearReg) +
                      strlen(move) + strlen(moveBack) + 5;
            char* assembly = malloc(len);
            snprintf(assembly, len, "%s\n%s\n%s\n%s\n%s",clearReg,loadAddr,
                    loadGadget.assembly,move,moveBack);
            free(clearReg);
            free(loadAddr);
            free(move);
            free(moveBack);
            free(varName);
            return assembly;
        }
        free(clearReg);
        free(loadAddr);
        free(move);
        free(moveBack);

        // if (used(dest,usedRegs,vars->count)) {
        //     moveRegAnywhere(dest, usedRegs, vars, gadgets);
        // } 
    }
    free(varName);
    return NULL;
}

// Attempt to move a var into dest for use - TODO rename?
char* checkRegisterPossible(Var* var, char* dest, Var* noMove, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets){
    // Vars* vars = *varsPtr;
    // char** usedRegs = *usedRegsPtr;
    // int count = vars->count;
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

char* synthesizeCopy(Copy inst, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    Var* dest = findVar(inst.dest, vars);
    
    // Copy number (must be hex 0x...)
    if (inst.src[0] == '0') {
        dest->value = (int)strtol(inst.dest, NULL, 0);
        strcpy(dest->reg, "new");
        dest->constant = true;
        dest->inMemory = false;
        return "";
    }

    Var* src = findVar(inst.src, vars);
    // If src is constant just update value as if fresh
    if (src->constant) {
        dest->value = src->value;
        strcpy(dest->reg, "new");
        dest->constant = true;
        dest->inMemory = false;
        return "";
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
        // printf("copy %s %s\n",inst.dest,inst.src);
        // TODO check for NULL - i.e. impossible
        char** usedRegs = usedRegisters(vars);
        // char** *usedRegsPtr = &usedRegs;
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
            // printf("gadget: %s\n",gadget.assembly);
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
    // printf("alt\n");
    // Couldn't find gadget so try to find alternative
    Vars* tmpVars = copyVars(vars);
    char* alt = findAlternative(inst, tmpVars, gadgets);
    if (alt == NULL) {
        return -1;  // Synthesis failed
    }
    // printf("%s\n",alt);
    // printf("%d\n",strlen(alt));
    int lines = 0;
    for (int i = 0 ; i < strlen(alt) ; i++) {
        if (alt[i] == '\n'){
            lines++;
        }
    }
    // printf("%s\n",alt);
    // printf("lines: %d\n",lines);
    // All non fresh vars should have lifespans longer than alt prog
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            tmpVars->vars[i]->lifeSpan = lines + 1;
        }
    }
    char* altProg[lines];
    getProgLines(altProg, alt);
    // for (int i = 0 ; i < lines ; i++){
    //     printf("%s\n",altProg[i]);
    // }
    Pseudo altPseudoInst[lines];
    createPseudo(lines, altProg, tmpVars, altPseudoInst);
    translatePseudo(lines, &tmpVars, altPseudoInst, gadgets);
    // printf("done alt\n");
    // Swap variables
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            Var* temp = vars->vars[i];
            (*varsPtr)->vars[i] = tmpVars->vars[i];
            (*varsPtr)->vars[i]->lifeSpan = temp->lifeSpan;  // Reset lifespan
            tmpVars->vars[i] = temp;
        }
    }
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
    // // Swap variables
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
            }
            else {
                printf("Warning could not store\n");
            }
            freeUsedRegs(usedRegs, count);
            free(assembly);
        }
    }
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
                        case '&':
                            findVar(inst.operand1,vars)->value &= findVar(inst.operand2,vars)->value;
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
                break;
            }
            case COMP: {
                Comp* inst = &pseudoInst[i].comp;
                storeAllVar(varsPtr, gadgets);
                vars = *varsPtr;
                // Jump over elseif/else if required
                if(inst->joinedIf != NULL) {
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
                synthesizeJump(inst, vars, gadgets);
                break;
            }
            case SPECIAL: {
                Special inst = pseudoInst[i].special;
                synthesizeSpecial(inst, varsPtr, gadgets);
                vars = *varsPtr;
                Var* v = findVar(inst.operand,vars);
                v->constant = false;    
                switch (inst.opcode) {
                    case '~': {
                        v->value = ~v->value;
                        v->value ++;
                        break;
                    }
                    case '!': 
                        v->value = ~v->value;
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
    // const int progLines = 9;
    enum { progLines = 3 };
    char* prog[progLines] = {
        "Var x 2",
        "Const y 3",

        "Mul x y"
    };
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

    vars->vars[0] = addressVar;
    vars->count = 1;
    Pseudo pseudoInst[progLines];
    
    // Parse program into pseudo instructions
    createPseudo(progLines, prog, vars, pseudoInst);
    // Read gadgets file
    Gadgets gadgets = loadGadgets();
    translatePseudo(progLines, &vars, pseudoInst, gadgets);
    printf("\n__Results__\n");
    for (int i = 1 ; i < vars->count ; i ++) {
        Var* v = vars->vars[i];
        printf("%s: %d\n",v->name,v->value);
    }
    freeVars(vars);
    freeGadgets(gadgets);
    freePseudo(progLines, pseudoInst);
    return 0;
}
