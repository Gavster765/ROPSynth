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

typedef struct Psuedo {
    GadgetType  type;  // Type of psuedo instruction
    LoadConst loadConst;
    ArithOp arithOp;
} Psuedo;