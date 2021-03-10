#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "var.h"
#include "utils.h"

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

Var* findVarByReg(char* reg, Vars* vars) {
    for (int i = 0 ; i < vars->count ; i++) {
        if (strcmp(vars->vars[i]->reg, reg) == 0) {
            return vars->vars[i];
        }
    }
    return NULL;
}

void updateLifespan(char* name, Vars* vars, int currLine, bool loop) {
    Var* v = findVar(name, vars);
    if (loop){
        v->loop = true;
    }
    else {
        v->lifeSpan = currLine+1;
    }
}

void updateLoopVars(Vars* vars, int currLine) {
    for (int i = 0 ; i < vars->count ; i++) {
        Var* v = vars->vars[i];
        if (v->loop){
            v->lifeSpan = currLine+1;
            v->loop = false;
        }
    }
}

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

        copy->vars[i] = newVar;
    }
    return copy;
}

void freeVars(Vars* vars){
    for (int i = 0 ; i < vars->count ; i++){
        free(vars->vars[i]);
    }
    free(vars);
}

void deleteStaleVars(int line, Vars* vars) {
    for (int i = 0 ; i < vars->count ; i++) {
        Var* v = vars->vars[i];

        if (v->lifeSpan == line || v->constant || v->inMemory){
            strcpy(v->reg, "new");
        }
    }
}