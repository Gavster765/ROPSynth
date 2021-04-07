#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "synthesis.h"
#include "./var.h"
#include "./gadgets.h"
#include "./pseudo.h"
#include "./utils.h"

// Rules for finding replacements that require loops
// Assumes operands are in _x and _y with result in _res
void staticSynthesis(Gadgets gadgets) {
    // Multiplication by var
    char* spec = strdup("Var,"
                        "Var,"
                        "Mul 0 1");

    char* synth = strdup("Var _res 0\n"
                         "Var _count 1\n"
                         "Const _one 1\n"
                         "While _count <= _y\n"
                            "Add _res _x\n"
                            "Add _count _one\n"
                         "End\n");
    addSynthComp(spec, synth, gadgets);
    // Division by var
    spec = strdup("Var,"
                  "Var,"
                  "Div 0 1");

    synth = strdup("Var _res 0\n"
                   "Const _one 1\n"
                   "Const _zero 0\n"
                   "Sub _x _y\n"
                   "While _x >= _zero\n"
                       "Add _res _one\n"
                       "Sub _x _y\n"
                   "End\n");
    addSynthComp(spec, synth, gadgets);
    // Modulus
    spec = strdup("Var,"
                  "Var,"
                  "Mod 0 1");

    synth = strdup("Const _res 0\n"
                   "Const _one 1\n"
                   "Const _zero 0\n"
                   "Sub _x _y\n"
                   "While _x >= _zero\n"
                       "Sub _x _y\n"
                   "End\n"
                   "Add _x _y\n"
                   "Copy _res _x\n");
    addSynthComp(spec, synth, gadgets);
}

char* findComponents(Gadgets gadgets) {
    char* components = malloc(gadgets.numArithOpGadgets * 4 * 3 + 1);  // Max 3 char op with comma
    components[0] = '\0';
    for (int i = 0 ; i < gadgets.numArithOpGadgets ; i++) {
        Gadget op = gadgets.arithOpGadgets[i];
        if (strcmp(op.operands[0], op.operands[1]) != 0) {
            sprintf(components, "%s%s,",components,op.opcode);
            // sprintf(components, "%s%s,",components,op.opcode);
            // sprintf(components, "%s%s,",components,op.opcode);
        }
    }
    int len = strlen(components);
    if (components[len-1] == ',') {
        components[len-1] = '\0';  // Remove trailing comma
    }
    // printf("%s\n",components);
    return components;
}

// Create prog spec without constants to check whether static exists
char* checkStatic(ArithOp inst, Vars* vars, Gadgets gadgets) {
    // for single inst will be var,var/const,op 0 1
    char* spec = malloc(40);  // Guess at max size
    spec[0] = '\0';
    sprintf(spec,"Var,Var,%s 0 1",inst.op);
    char* synth = getSynth(spec, gadgets);
    free(spec);
    return synth;
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
        addVar(freshName, vars);
        
        char* opcode = strtok(newInst, " ");  // Peel off opcode
        opcode[0] = toupper(opcode[0]);
        char* operands = strtok(NULL, "");  // Save all operands
        char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
        // printf("%s %s\n",opcode, operands);
        int num = getGadgetOperands(operandList,operands);

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
        if (num > 0){
            free(operandList[0]);
        }
        free(operandList);
    }
    sprintf(pseudoInst,"%sCopy %s _%s\n",pseudoInst,inst.operand1,varName);
    free(varName);
    free(freshName);
    free(newInst);
    return pseudoInst;
}

char* replaceVars(char* synth, ArithOp inst, Vars* vars) {
    char* setup = malloc(50 + strlen(inst.operand1) + strlen(inst.operand2));
    sprintf(setup, 
        "Const _x 0\n"
        "Const _y 0\n"
        "Copy _x %s\n"
        "Copy _y %s\n",
        inst.operand1,inst.operand2
    );
    char* finish = malloc(20 + strlen(inst.operand1));
    sprintf(finish,
        "Copy %s _res\n",
        inst.operand1
    );
    char* completeSynth = malloc(strlen(synth) + strlen(setup) + strlen(finish) + 1);
    sprintf(completeSynth,"%s%s%s",setup,synth,finish);
    free(setup);
    free(finish);
    return completeSynth;
}

char* findAlternative(ArithOp inst, Vars* vars, Gadgets gadgets) {
    char* staticSynth = checkStatic(inst, vars, gadgets);
    if (staticSynth != NULL) {
        char* synth = replaceVars(staticSynth, inst, vars);
        return synth;
    }
    
    char* components = findComponents(gadgets);
    char* spec = createProgSpec(inst, vars);
    char* synth = getSynth(spec, gadgets);
    if (synth == NULL) {
        synth = run(components, spec);
        if (strcmp(synth,"Error") == 0) {
            free(spec);
            free(components);
            free(synth);
            return NULL; // Synthesis failed
        }
        addSynthComp(spec, synth, gadgets);
        synth = parseNewProg(synth, inst, vars);
    }
    else {
        free(spec);
        synth = parseNewProg(synth, inst, vars);
    }
    free(components);
    return synth;
}
