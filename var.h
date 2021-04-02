#ifndef VAR_H
#define VAR_H

#include <stdbool.h>

// Consider changing data structure
typedef struct Var {
    int value;
    int lifeSpan;  // from this line on don't need var
    bool loop;  // used inside loop so don't kill until loop end
    bool constant;  // not changed so can be reloaded from stack at will
    bool inMemory;
    bool address;  // Marks placeholder variable 
    int memAddress;
    char reg[4];
    char name[];
} Var;

typedef struct Vars {
    int count;
    int maxSize;
    Var* vars[];
} Vars;

// Methods of finding a var from a given vars structure - return NULL on fail

Var* findVar(char* name, Vars* vars);
Var* findVarByReg(char* reg, Vars* vars);

// Add and delete from vars

int addNewVar(Var* newVar, Vars* vars);
Var* addVar(char* name, Vars* vars);
int removeVar(Var* delVar, Vars* vars);

// Used during createPseudo phase to calculate variable lifespan

void updateLifespan(char* name, Vars* vars, int currLine, bool loop);
void updateLoopVars(Vars* vars, int currLine);

// Create and delete copies of vars

Vars* copyVars(Vars* vars);
void freeVars(Vars* vars);
void deleteStaleVars(int line, Vars* vars);

#endif /* VAR_H */
