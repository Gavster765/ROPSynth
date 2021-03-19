#include <stdio.h>
#include "synthesis.h"
#include "./var.h"
#include "./gadgets.h"
#include "./pseudo.h"

void findAlternative(ArithOp inst, Vars* var, Gadgets gadgets) {
    char* res = run("Add,Add,Add,And,Sub,Xor", "Var,Const 4,Mul 0 1");
    printf("%s\n",res);
}