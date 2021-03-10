#include <stdbool.h>
#include "var.h"

void removeChars(char* str, char c);

int getGadgetOperands(char** operandList, char* operandString);
int getOperands(char** operandList, char* operandString);

int getProgLines(char** progList, char* progString);

bool used(char* reg, char** usedRegs, int count);

char** usedRegisters(Vars* vars);
void freeUsedRegs(char** usedRegs, int count);
void addRegToUsed(char** used, char* reg, int count);
void removeRegFromUsed(char** used, char* reg, int count);