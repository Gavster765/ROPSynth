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
            int numOperands = getOperands(operandList, operands);
            if(strcmp(opcode,"Var") == 0){
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
            // TODO move check if arith to func?
            else if(strcmp(opcode,"Add") == 0 || strcmp(opcode,"Sub") == 0 || strcmp(opcode,"Mul") == 0){
                ArithOp newArith = {
                    .out = operandList[0],
                    .operand1 = operandList[0],
                    .operand2 = operandList[1]
                };

                if(strcmp(opcode,"Add") == 0) {
                    newArith.opcode = '+';
                    newArith.op = "Add";
                }
                else if(strcmp(opcode,"Sub") == 0) {
                    newArith.opcode = '-';
                    newArith.op = "Sub";
                }
                else if(strcmp(opcode,"Mul") == 0) {
                    newArith.opcode = '*';
                    newArith.op = "Mul";
                }

                Pseudo p = {
                    .type = ARITH_OP,
                    .arithOp = newArith
                };
                pseudoInst[i] = p;
                updateLifespan(operandList[0], vars, i, loop);
                updateLifespan(operandList[1], vars, i, loop);
            }
            else if(strcmp(opcode,"Copy") == 0) {
                Copy newCopy = {
                    .dest = operandList[0],
                    .src = operandList[1]
                };

                Pseudo p = {
                    .type = COPY,
                    .copy = newCopy
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
    char* varName = strdup(var->name);
    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;

    if (strcmp(var->reg, dest) == 0){
        return "";
    }
    // WARNING assumes no move gagdet longer than first and MEM LEAK?
    char* assembly;// = malloc(2 * strlen(gadgets.moveRegGadgets[0].assembly) + 2);
    if (used(dest, usedRegs, vars->count)){
        char* moveExisting = moveRegAnywhere(dest, usedRegsPtr, varsPtr, gadgets);
        vars = *varsPtr;
        var = findVar(varName, vars);
        usedRegs = *usedRegsPtr;
        if (moveExisting == NULL){
            return NULL;
        }
        assembly = malloc(strlen(gadgets.moveRegGadgets[0].assembly) +
                          strlen(moveExisting) + 2);
        assembly[0] = '\0';    
        strcat(assembly,moveExisting);
        strcat(assembly,"\n");
    }
    else {
        assembly = malloc(2 * strlen(gadgets.moveRegGadgets[0].assembly) + 2);
        assembly[0] = '\0';    
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
    free(varName);
    return NULL;
}

// TODO move then load?
// Will  try to load a const from the stack into dest - can move other var out of dest
char* loadConstValue(Var* var, char* dest, char** *usedRegsPtr, Vars* *varsPtr, Gadgets gadgets){
    Vars* vars = *varsPtr;
    char** usedRegs = *usedRegsPtr;
    int count = vars->count;

    for (int i = 0 ; i < gadgets.numLoadConstGadgets ; i++) {
        Gadget loadGadget = gadgets.loadConstGadgets[i];
        if (used(dest,usedRegs,count)){
            break;  // Reg in use - would clobber value - move first?
        }
        else if (strcmp(loadGadget.operands[0],dest) == 0){
            strcpy(var->reg, dest);
            addRegToUsed(usedRegs, dest, vars->count);
            char* assembly = malloc(strlen(loadGadget.assembly) + sizeof(int) + 1);
            sprintf(assembly, "%s (%d)",loadGadget.assembly,var->value);  // TODO - free
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
                int len = strlen(loadGadget.assembly) + strlen(possMove) + sizeof(int) + 2;
                char* assembly = malloc(len);
                assembly[0] = '\0';
                snprintf(assembly, len, "%s (%d)\n%s", loadGadget.assembly, tmpVar->value, possMove);
                freeVars(vars);
                freeUsedRegs(usedRegs, count);
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
    Var* addressVar = NULL;
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
            addressVar = vars->vars[0];
            addressVar->value = var->memAddress;
            loadAddr = loadConstValue(addressVar, storeAddr, usedRegsPtr, varsPtr, gadgets);
            if (loadAddr == NULL) {
                continue;
            }
            usedRegs = *usedRegsPtr;
            vars = *varsPtr;
        }
        // printf("store var: %s\n",varName);
        char* moveData = moveReg(findVar(varName, vars), storeData, usedRegsPtr, varsPtr, gadgets);

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
            if (addressVar != NULL){
                strcpy(addressVar->name, "new");
            }
            // printf("store part: %s\n",assembly);
            // printf("done\n");
            free(varName);
            return assembly;
        }
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
    }

    for (int i = 0 ; i < gadgets.numLoadMemGadgets ; i++) {
        Gadget loadGadget = gadgets.loadMemGadgets[i];
        char* loadDest = loadGadget.operands[0];
        char* srcAddr = loadGadget.operands[1];
        char* clearReg;
        char* loadAddr;
        char* move;
        char* moveBack;

        // Only gadgets have srcAddr = dest for now .. TODO
        if (used(srcAddr,*usedRegsPtr,(*varsPtr)->count)) {
            clearReg = moveRegAnywhere(srcAddr, usedRegsPtr, varsPtr, gadgets);
        }
        else {
            clearReg = "";
        }

        if (clearReg != NULL) {
            loadAddr = loadConstValue(findVar(varName,*varsPtr), srcAddr, usedRegsPtr, varsPtr, gadgets);
        }

        if (loadAddr != NULL) {
            var = findVar(varName, *varsPtr);
            strcpy(var->reg, loadDest);
            move = moveReg(var, dest, usedRegsPtr, varsPtr, gadgets);
        }
        
        Var* noMoveVar = findVarByReg(noMoveVarName, *varsPtr);
        if (noMoveVar != NULL) {
            if (strcmp(noMoveVarReg, noMove->reg) != 0){
                // TODO - combine with prev move? so can make move safe
                moveBack = moveReg(noMoveVar, noMoveVarReg, usedRegsPtr, varsPtr, gadgets);
            }
            else {
                moveBack = "";
            }
            free(noMoveVarName);
        }
        else {
            moveBack = "";
        }
        
        if (clearReg != NULL && loadAddr != NULL && move != NULL && moveBack != NULL){
            int len = strlen(loadGadget.assembly) + strlen(loadAddr) + strlen(clearReg) +
                      strlen(move) + strlen(moveBack) + 5;
            char* assembly = malloc(len);
            snprintf(assembly, len, "%s\n%s\n%s\n%s\n%s",clearReg,loadAddr,
                    loadGadget.assembly,move,moveBack);
            free(varName);
            return assembly;
        }
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
        return "";  // No change needed
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
    Var* src = findVar(inst.src, vars);

    // If src is constant just update value as if fresh
    if (src->constant) {
        dest->value = src->value;
        strcpy(dest->reg, "new");
        dest->constant = true;
        dest->inMemory = false;
        return "";
    }
    // Case in reg - TODO memory case
    else {
        // TODO check for NULL - i.e. impossible
        char** usedRegs = usedRegisters(vars);
        dest->value = src->value;
        dest->constant = false;
        dest->inMemory = false;
        strcpy(dest->reg, src->reg);
        return moveRegAnywhere(src->reg, &usedRegs, varsPtr, gadgets);
    }
}

// Create type a=a+b
// Write asm for arithmetic operations
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
             (op == '-' &&  strcmp(gadget.opcode,"sub") == 0) ||
             (op == '*' &&  strcmp(gadget.opcode,"mul") == 0)  ) {
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
                return;
            }
        }
        freeUsedRegs(usedRegs, count);
        freeVars(tmpVars);
    }
    printf("alt\n");
    // Couldn't find gadget so try to find alternative
    Vars* tmpVars = copyVars(vars);
    char* alt = findAlternative(inst, tmpVars, gadgets);
    // printf("%s\n",alt);
    // printf("%d\n",strlen(alt));
    int lines = 0;
    for (int i = 0 ; i < strlen(alt) ; i++) {
        if (alt[i] == '\n'){
            lines++;
        }
    }
    // printf("%s\n",alt);
    char* altProg[lines];
    getProgLines(altProg, alt);
    for (int i = 0 ; i < lines ; i++){
        printf("%s\n",altProg[i]);
    }
    Pseudo altPseudoInst[lines];
    createPseudo(lines, altProg, tmpVars, altPseudoInst);
    translatePseudo(lines, &tmpVars, altPseudoInst, gadgets);
    printf("done alt\n");
    // Swap variables
    for (int i = 0 ; i < tmpVars->count ; i ++) {
        if (tmpVars->vars[i]->name[0] != '_') {
            Var* tmp = vars->vars[i];
            vars->vars[i] = tmpVars->vars[i];
            tmpVars->vars[i] = tmp;
        }
    }
    freeVars(tmpVars);

    // TODO - execute new program - need more space first? (memory stores)
}

// Read list of pseudo instructions and write required asm
void translatePseudo(int progLines, Vars* *varsPtr, Pseudo* pseudoInst, Gadgets gadgets){
    for (int i = 0 ; i < progLines ; i++){
        Vars* vars = *varsPtr;
        deleteStaleVars(i, vars);
        switch (pseudoInst[i].type){
            case LOAD_CONST: {
                LoadConst inst = pseudoInst[i].loadConst;
                findVar(inst.out,vars)->value = inst.value;
                break;
            }
            case ARITH_OP: {
                ArithOp inst = pseudoInst[i].arithOp;
                synthesizeArith(inst, varsPtr, gadgets);
                vars = *varsPtr;
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
                }
                break;
            }
            case COPY: {
                Copy inst = pseudoInst[i].copy;
                char* copy = synthesizeCopy(inst, varsPtr, gadgets);                if (copy == NULL) {
                    printf("Can't copy\n");
                }
                printf("%s\n",copy);
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
    const int progLines = 4;
    char* prog[progLines] = {
        "Var x 2",
        "Var y 4",
        "Add x y",
        "Mul x y"
    };
    // Allocate space for variables and pseudo instructions
    Vars *vars = malloc(sizeof(Vars) + sizeof(Var*)*(progLines+4));
    vars->count = 0;
    vars->maxSize = progLines+4;

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
    
    return 0;
}