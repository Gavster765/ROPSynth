#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "synthesis.h"
#include "./var.h"
#include "./gadgets.h"
#include "./pseudo.h"

char* findComponents(Gadgets gadgets) {
    char* components = malloc(gadgets.numArithOpGadgets * 4 + 1);  // Max 3 char op with comma
    components[0] = '\0';
    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++) {
        Gadget op = gadgets.arithOpGadgets[i];
        if (strcmp(op.operands[0], op.operands[1]) != 0) {
            sprintf(components, "%s%s,",components,op.opcode); // TODO remove last
        }
    }
    printf("%s\n",components);
    return components;
}

char* createProgSpec(ArithOp inst, Vars* vars) {
    // for single inst will be var,var/const,op 0 1
    char* spec = malloc(40);  // Guess at max size
    spec[0] = '\0';
    strcat(spec,"Var,");

    Var* b = findVar(inst.operand2, vars);
    if (b->constant) {
        sprintf(spec, "%sConst %d,",spec,b->value);
    }
    else {
        strcat(spec,"Var,");
    }

    strcat(spec, inst.op);
    strcat(spec, " 0 1");
    printf("%s\n",spec);
    return spec;
}

char* parseNewProg(char* prog, Vars* vars) {
    char* varName = malloc(2);
    char* inst = malloc(30);

    char* curr = prog;
    while (curr) {
        char* next = strchr(curr, '\n');
        if (next) next[0] = '\0';  // temporarily terminate the current line
        
        // printf("%s\n",curr);
        int read = sscanf(curr,"%s ← %[^\n]",varName,inst);
        if (read == -1) {
            break;
        }
        printf("%s %s\n",varName,inst);
        Var *v = malloc(sizeof(Var) + strlen("_x") + 1);
        strcpy(v->name,"_");
        strcat(v->name,varName);
        addNewVar(v, vars);

        if (next) *next = '\n';  // then restore newline-char, just to be tidy    
        curr = next ? (next+1) : NULL;
    }
    free(varName);
    free(inst);
    return "";
}

char* findAlternative(ArithOp inst, Vars* vars, Gadgets gadgets) {
    char* components = findComponents(gadgets);
    char* spec = createProgSpec(inst, vars);
    // char* res = run("add,add,add,and,sub,xor,", "Var,Const 4,Mul 0 1");
    char* res = run(components, spec);
    // printf("%s\n",res);
    parseNewProg(res, vars);
    for (int i = 0 ; i < vars->count ; i++){
        printf("%s\n",vars->vars[i]->name);
    }
    
    // Var *var2 = malloc(sizeof(Var) + strlen("b") + 1);
    // strcpy(var2->name,"b");
    // addNewVar(var, vars);
    // addNewVar(var2, vars);
    // removeVar(var, vars);
    free(components);
    free(spec);
    return res;
}