#include "gadgets.h"

typedef struct LoadConst {
    char* out;
    int value;
} LoadConst;

typedef struct ArithOp {
    char opcode;
    char* out;
    char* operand1;
    char* operand2;
} ArithOp;

typedef struct Comp {
    char* opcode;
    char* operand1;
    char* operand2;
    int end;
    bool valid;
    struct Comp* joinedIf;
} Comp;

typedef struct Pseudo {
    GadgetType type;  // Type of pseudo instruction
    LoadConst loadConst;
    ArithOp arithOp;
    Comp comp;
} Pseudo;