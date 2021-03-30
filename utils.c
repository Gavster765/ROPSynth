#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "utils.h"

// Remove all occurrences of c from str
void removeChars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

// Work out the registers used by the gadget
int getGadgetOperands(char** operandList, char* operandString) {
    if (operandString == NULL) {
        return 0;
    }

    char* operands = strdup(operandString);
    char* operand = strtok(operands, ",");
    int numOperands = 0;
    // Iterate though operands 
    while(operand != NULL){
        removeChars(operand,' ');
        // For now ignore length of data read/written
        int a = strspn(operand, "DWORDPTR[");
        if (a == 9){
            memcpy(operand, operand+9, 3);
            operand[3] = '\0';
        }
        operandList[numOperands] = operand;
        numOperands++;
        operand = strtok(NULL, ",");
    }
    return numOperands;
}

// Unpack the operands of a user program instruction
int getOperands(char** operandList, char* operandString) {
    if (operandString == NULL){
        return 0;
    }
    char* operands = strdup(operandString);
    char* operand = strtok(operands, " ");
    int numOperands = 0;
    // Iterate though operands 
    while(operand != NULL){
        removeChars(operand,' ');
        operandList[numOperands] = operand;
        numOperands++;
        operand = strtok(NULL, " ");
    }
    return numOperands;
}

int getProgLines(char** progList, char* progString) {
    if (progString == NULL){
        return -1;
    }
    char* progLine = strtok(progString, "\n");
    int numLines = 0;
    // Iterate though operands 
    while(progLine != NULL){
        progList[numLines] = progLine;
        numLines++;
        progLine = strtok(NULL, "\n");
    }
    return numLines;
}

// Check if a register exists withing usedRegs
bool used(char* reg, char** usedRegs, int count){
    for (int i = 0 ; i < count ; i++){
        if(strcmp(usedRegs[i],reg) == 0){
            return true;
        }
    }
    return false;
}

// return a list of registers currently in use excluding constants
char** usedRegisters(Vars* vars){
    char** usedRegs = malloc(vars->count * sizeof(char*));
    for (int i = 0 ; i < vars->count ; i++) {
        usedRegs[i] = malloc(4);
        if(vars->vars[i]->constant || vars->vars[i]->inMemory){
            strcpy(usedRegs[i], "new");  // Allow constant to be overwritten
        }
        else {
            strcpy(usedRegs[i], vars->vars[i]->reg);
        }
    }
    return usedRegs;
}

// Free memory used by usedRegs struct
void freeUsedRegs(char** usedRegs, int count){
    for (int i = 0 ; i < count ; i++) {
        free(usedRegs[i]);    
    }
    free(usedRegs);
}

// Add a register to the list of used registers
void addRegToUsed(char** usedRegs, char* reg, int count){
    if(!used(reg,usedRegs,count)){
        for (int i = 0 ; i < count ; i++) {
            if(strcmp(usedRegs[i],"new") == 0) {
                strcpy(usedRegs[i],reg);
                return;
            }
        }
    }
}

// Remove a register from the list of used registers
void removeRegFromUsed(char** usedRegs, char* reg, int count){
    if(used(reg,usedRegs,count)){
        for (int i = 0 ; i < count ; i++) {
            if(strcmp(usedRegs[i],reg) == 0) {
                strcpy(usedRegs[i],"new");
                return;
            }
        }
    }
}

char getOpChar(char* opcode) {
    char first = opcode[0];
    char op = '_';
    opcode[0] = tolower(opcode[0]);
    if(strcmp(opcode,"add") == 0) {
        op = '+';
    }
    else if(strcmp(opcode,"sub") == 0) {
        op = '-';
    }
    else if(strcmp(opcode,"mul") == 0) {
        op = '*';
    }
    else if(strcmp(opcode,"and") == 0) {
        op = '&';
    }
    opcode[0] = first;
    return op;
}

bool checkArithOp(char* opcode) {
    bool isArith = false;
    if (getOpChar(opcode) != '_') {
        isArith = true;
    }
    return isArith;
}

bool checkArithOpGadget(char opcode, char* gadget) {
    bool gadgetCorrect = false;
    if (getOpChar(gadget) == opcode) {
        gadgetCorrect = true;
    }
    return gadgetCorrect;
}

void fillArithOp(ArithOp* arithOp, char* opcode) {
    char op = getOpChar(opcode);
    if (op != '_') {
        char first = opcode[0];
        opcode[0] = toupper(opcode[0]);
        arithOp->opcode = op;
        arithOp->op = opcode;
        opcode[0] = first;
    }
}