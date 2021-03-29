#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "synthesis.h"
#include "./var.h"
#include "./gadgets.h"
#include "./pseudo.h"
#include "./utils.h"

char* findComponents(Gadgets gadgets) {
    char* components = malloc(gadgets.numArithOpGadgets * 4 + 1);  // Max 3 char op with comma
    components[0] = '\0';
    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++) {
        Gadget op = gadgets.arithOpGadgets[i];
        if (strcmp(op.operands[0], op.operands[1]) != 0) {
            sprintf(components, "%s%s,",components,op.opcode);
        }
    }
    int len = strlen(components);
    if (components[len-1] == ',') {
        components[len-1] = '\0';
    }
    // printf("%s\n",components);
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
    // printf("%s\n",spec);
    return spec;
}

char* parseNewProg(char* prog, ArithOp inst, Vars* vars) {
    char* pseudoInst = malloc(strlen(prog)+100); // TODO calculate actual length
    pseudoInst[0] = '\0';
    char* varName = malloc(2);
    char* freshName = malloc(3);
    char* newInst = malloc(30);
    bool firstVar = true;

    char* curr = prog;
    while (curr) {
        char* next = strchr(curr, '\n');
        if (next) next[0] = '\0';  // temporarily terminate the current line
        
        // printf("%s\n",curr);
        int read = sscanf(curr,"%s ← %[^\n]",varName,newInst);
        if (read == -1) {
            break;
        }
        // printf("%s %s\n",varName,inst);
        strcpy(freshName, "_");
        strcat(freshName, varName);
        Var* v = addVar(freshName, vars);
        
        char* opcode = strtok(newInst, " ");  // Peel off opcode
        opcode[0] = toupper(opcode[0]);
        char* operands = strtok(NULL, "");  // Save all operands
        char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
        // printf("%s %s\n",opcode, operands);
        getGadgetOperands(operandList,operands);

        if (strcmp("Var",opcode) == 0) {
            if (firstVar) {
                sprintf(pseudoInst,"%sCopy _%s %s\n",pseudoInst,varName,inst.operand1);
                firstVar = false;
            }
            else {
                sprintf(pseudoInst,"%sCopy _%s %s\n",pseudoInst,varName,inst.operand2);
            }
            // strcat(pseudoInst,"Copy");
        }
        else if (strcmp("Const",opcode) == 0) {
            sprintf(pseudoInst,"%sCopy _%s %s\n",pseudoInst,varName,operandList[0]);
        }
        else {
            sprintf(pseudoInst,"%sCopy _%s _%s\n",pseudoInst,varName,operandList[0]);
            sprintf(pseudoInst,"%s%s _%s _%s\n",pseudoInst,opcode,varName,operandList[1]);
            // printf("%s %s %s\n",opcode,operandList[0],operandList[1]);
        }

        if (next) *next = '\n';  // then restore newline-char, just to be tidy    
        curr = next ? (next+1) : NULL;
    }
    sprintf(pseudoInst,"%sCopy %s _%s\n",pseudoInst,inst.operand1,varName);
    free(varName);
    free(freshName);
    free(newInst);
    return pseudoInst;
}

char* findAlternative(ArithOp inst, Vars* vars, Gadgets gadgets) {
    char* components = findComponents(gadgets);
    char* spec = createProgSpec(inst, vars);
    // char* res = run("add,add,add,and,sub,xor,", "Var,Const 4,Mul 0 1");
    char* res = run(components, spec);
    if (strcmp(res,"Error") == 0) {
        return NULL; // Synthesis failed
    }
    char* pseudoCode = parseNewProg(res, inst, vars);
    // printf("%s\n",pseudoCode);
    // for (int i = 0 ; i < vars->count ; i++){
    //     printf("%s\n",vars->vars[i]->name);
    // }
    
    // Var *var2 = malloc(sizeof(Var) + strlen("b") + 1);
    // strcpy(var2->name,"b");
    // addNewVar(var, vars);
    // addNewVar(var2, vars);
    // removeVar(var, vars);
    free(components);
    free(spec);
    return pseudoCode;
}