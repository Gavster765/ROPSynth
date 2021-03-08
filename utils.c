#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "var.h"

// Remove all occurrences of c from str
void removeChars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

int getGadgetOperands(char** operandList, char* operandString) {
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


bool exists(char* reg, char** usedRegs, int count){
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
        if(vars->vars[i]->constant){
            strcpy(usedRegs[i], "new");  // Allow constant to be overwritten
        }
        else {
            strcpy(usedRegs[i], vars->vars[i]->reg);
        }
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