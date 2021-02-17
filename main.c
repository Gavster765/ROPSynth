#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gadgets.h"
#include "utils.h"

typedef struct Var {
    char* name;
    int value;
} Var;

int main(){
    // Gadgets gadgets = loadGadgets();
    int progLines = 3;
    char* prog[3] = {
        "Var x, 1",
        "Var y, 2",
        "Add x, y"
    };
    Var vars[progLines];

    for (int i = 0 ; i < progLines ; i++){
        char* line = strdup(prog[i]);
        char* opcode = strtok(line, " ");
        char* operands = strtok(NULL, "");  // Save all operands
        char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
        int numOperands = getOperands(operandList, operands);
        
        if(strcmp(opcode,"Var") == 0){
            Var newVar = {
                operandList[0],
                atoi(operandList[1])
            };
            vars[i] = newVar;  // Use hastable? Also probs shouldn't save value yet??
        }
    }
    printf("%d %d\n",vars[0].value,vars[1].value);
    return 0;
}