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
    if (operandString == NULL) {
        return 0;
    }

    char* operands = strdup(operandString);
    char* operand = strtok(operands, ",");
    int numOperands = 0;
    // Iterate though operands 
    while(operand != NULL){
        removeChars(operand,' ');
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