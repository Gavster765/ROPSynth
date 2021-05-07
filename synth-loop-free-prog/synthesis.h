#include "./var.h"
#include "./gadgets.h"
#include "./pseudo.h"
// Linked rust function
char* run(char* components, char* program);

char* findAlternative(Pseudo inst, Vars* vars, Gadgets gadgets);

void staticSynthesis(Gadgets gadgets);
