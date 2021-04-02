#include "./var.h"
#include "./gadgets.h"
#include "./pseudo.h"
// Linked rust function
char* run(char* components, char* program);

char* findAlternative(ArithOp inst, Vars* vars, Gadgets gadgets);

void staticSynthesis(Gadgets gadgets);
