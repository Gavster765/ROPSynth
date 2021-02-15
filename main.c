#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gadgets.h"

int main(){
    // Gadgets gadgets = loadGadgets();
    int progLines = 3;
    char* prog[3] = {
        "Var x, 1",
        "Var y, 2",
        "Add x, y"
    };
    char* vars[progLines];

    for (int i = 0 ; i < progLines ; i++){
        char* line = strdup(prog[i]);
        char* opcode = strtok(line, " ");
        // Consider refactoring to function
        char* operands = strtok(NULL, "");  // Save all operands
        char* operand = strtok(operands, ",");
        char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
        int numOperands = 0;
        // Iterate though operands 
        while(operand != NULL){
            removeChars(operand,' ');
            operandList[numOperands] = operand;
            numOperands++;
            operand = strtok(NULL, ",");
        }
        
        if(strcmp(opcode,"Var") == 0){
            vars[i] = operandList[1];  // Use hastable?
        }
    }
    printf("%s %s\n",vars[0],vars[1]);
    return 0;
}