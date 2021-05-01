#ifndef PSEUDO_H
#define PSEUDO_H

#include "gadgets.h"
#include <inttypes.h>

typedef struct LoadConst {
    char* out;
    int64_t value;
    bool instLoad;
} LoadConst;

typedef struct ArithOp {
    char opcode;
    char* op;
    char* out;
    char* operand1;
    char* operand2;
} ArithOp;

typedef struct Comp {
    char* opcode;
    char* operand1;
    char* operand2;
    bool loop;
    int start;  // Jump location for while loops
    int end;  // Jump location if not true
    int finish;  // Jump location for end of if else chain
    struct Comp* joinedIf;
    struct Comp* and;
} Comp;

typedef struct End {
    Comp* loop;
} End;

typedef struct Copy {
    char* dest;
    char* src;
} Copy;

typedef struct Jump {
    int dest;
    int* breakDest;
    char* opcode;
    char* operand1;
    char* operand2;
} Jump;

typedef struct Special {
    char opcode;
    char* op;
    char* operand;
} Special;

typedef struct Pseudo {
    GadgetType type;  // Type of pseudo instruction
    LoadConst loadConst;
    ArithOp arithOp;
    Copy copy;
    Comp comp;
    End end;
    Jump jump;
    Special special;
} Pseudo;

#endif /* PSEUDO_H */
