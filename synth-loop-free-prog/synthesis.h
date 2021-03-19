#include "./var.h"
#include "./gadgets.h"
#include "./pseudo.h"
// Linked rust function
char* run(char* components, char* program);

void findAlternative(ArithOp inst, Vars* vars, Gadgets gadgets);