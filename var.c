#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "var.h"
#include "utils.h"

// Find variable by name
Var* findVar(char* name, Vars* vars) {
    if (name != NULL) {
        for (int i = 0 ; i < vars->count ; i++) {
            if (strcmp(vars->vars[i]->name, name) == 0) {
                return vars->vars[i];
            }
        }
    }
    return NULL;
}

// Find variable which currently occupies a register
Var* findVarByReg(char* reg, Vars* vars) {
    for (int i = 0 ; i < vars->count ; i++) {
        if (strcmp(vars->vars[i]->reg, reg) == 0) {
            return vars->vars[i];
        }
    }
    return NULL;
}

// Updates the lifespan of var whenever it is referenced
void updateLifespan(char* name, Vars* vars, int currLine, bool loop) {
    Var* v = findVar(name, vars);
    if (loop){
        v->loop = true;
    }
    else {
        v->lifeSpan = currLine+1;
    }
}

// Updates the lifespan of all vars to the end of current loop
void updateLoopVars(Vars* vars, int currLine) {
    for (int i = 0 ; i < vars->count ; i++) {
        Var* v = vars->vars[i];
        if (v->loop){
            v->lifeSpan = currLine+1;
            v->loop = false;
        }
    }
}

// Create an exact copy of every var
Vars* copyVars(Vars* vars){
    Vars* copy = malloc(sizeof(Vars) + sizeof(Var*)*vars->count);
    copy->count = vars->count;
    for (int i = 0 ; i < vars->count ; i++){
        Var* var = vars->vars[i];
        Var *newVar = malloc(sizeof(Var) + strlen(var->name) + 1);
        strcpy(newVar->name, var->name);
        strcpy(newVar->reg, var->reg);
        newVar->value = var->value;
        newVar->lifeSpan = var->lifeSpan;
        newVar->loop = var->loop;  // Not used?
        newVar->constant = var->constant;
        newVar->inMemory = var->inMemory;
        newVar->memAddress = var->memAddress;

        copy->vars[i] = newVar;
    }
    return copy;
}

// Free all memory allocated to vars
void freeVars(Vars* vars){
    for (int i = 0 ; i < vars->count ; i++){
        free(vars->vars[i]);
    }
    free(vars);
}

// Delete all vars no longer needed in registers
void deleteStaleVars(int line, Vars* vars) {
    for (int i = 0 ; i < vars->count ; i++) {
        Var* v = vars->vars[i];

        if (v->lifeSpan == line || v->constant || v->inMemory){
            strcpy(v->reg, "new");
        }
    }
}