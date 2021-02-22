#include <stdlib.h>
#include <string.h>
#include "var.h"

Var* findVar(char* name, Vars* vars) {
    for (int i = 0 ; i < vars->count ; i++) {
        if (strcmp(vars->vars[i]->name, name) == 0) {
            return vars->vars[i];
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
        if (vars->vars[i]->lifeSpan == line){
            strcpy(vars->vars[i]->reg, "new");
        }
    }
}