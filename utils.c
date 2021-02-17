#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Remove all occurances of c from str
void removeChars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

int getOperands(char** operandList, char* operandString) {
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