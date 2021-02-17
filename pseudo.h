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

typedef struct Pseudo {
    GadgetType  type;  // Type of pseudo instruction
    LoadConst loadConst;
    ArithOp arithOp;
} Pseudo;