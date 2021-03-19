#include <stdio.h>
#include "test_lib.h"

int main() {
    char* res = run("Add,Add,Add,And,Sub,Xor", "Var,Const 4,Mul 0 1");
    printf("%s\n",res);
    return 0;
}