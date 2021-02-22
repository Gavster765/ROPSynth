// Consider changing data structure
typedef struct Var {
    int value;
    int lifeSpan;  // from this line on don't need var
    char reg[4];
    char name[];
} Var;

typedef struct Vars {
    int count;
    Var* vars[];
} Vars;

Var* findVar(char* name, Vars* vars);
Var* findVarByReg(char* reg, Vars* vars);

Vars* copyVars(Vars* vars);
void freeVars(Vars* vars);
void deleteStaleVars(int line, Vars* vars);