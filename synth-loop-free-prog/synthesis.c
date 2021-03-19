#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "synthesis.h"
#include "./var.h"
#include "./gadgets.h"
#include "./pseudo.h"

void findComponents(Gadgets gadgets) {
    char* components = malloc(gadgets.numArithOpGadgets * 3);
    components[0] = '\0';
    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++) {
        Gadget op = gadgets.arithOpGadgets[i];
        if (strcmp(op.operands[0], op.operands[1]) == 0) {
            // continue;  // Don't consider gadgets with matching operands
        }
        else {
            strcat(components, op.opcode);
            strcat(components, ",");
        }
    }
    printf("%s\n",components);
    free(components);
}

void findAlternative(ArithOp inst, Vars* var, Gadgets gadgets) {
    findComponents(gadgets);
    char* res = run("add,add,add,and,sub,xor", "var,const 4,mul 0 1");
    printf("%s\n",res);
}