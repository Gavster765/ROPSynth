#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    LOAD_CONST,
    MOVE_REG,
    BINARY_OP
} GadgetType;

typedef struct {
    GadgetType type;  // Type of gadget
    char* assembly;  // Actual string
    char* opcode;
    int numOperands;
    char** operands;
} Gadget;

// Remove all occurances of c from str
void removeChars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

Gadget createGadget(GadgetType type, const char* assembly){
    char* line = strdup(assembly);  // Make string writable
    char* opcode = strtok(line, " ");  // Peel off opcode
    char* operands = strtok(NULL, "");  // Save all operands

    char* operand = strtok(operands, ",");
    char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
    int numOperands = 0;
    // Iterate though operands 
    while(operand != NULL){
        removeChars(operand,' ');
        operandList[numOperands] = operand;
        numOperands++;
        operand = strtok(NULL, ",");
    }

    Gadget gadget = {
        type,
        line,
        opcode,
        numOperands,
        operandList
        };
    return gadget;
}

int main(){
    Gadget loadConstGadgets[8];
    loadConstGadgets[0] = createGadget(LOAD_CONST,"pop eax");
    loadConstGadgets[1] = createGadget(LOAD_CONST,"pop ecx");
    loadConstGadgets[2] = createGadget(LOAD_CONST,"pop edx");
    loadConstGadgets[3] = createGadget(LOAD_CONST,"pop ebx");
    loadConstGadgets[4] = createGadget(LOAD_CONST,"pop esp");
    loadConstGadgets[5] = createGadget(LOAD_CONST,"pop ebp");
    loadConstGadgets[6] = createGadget(LOAD_CONST,"pop esi");
    loadConstGadgets[7] = createGadget(LOAD_CONST,"pop edi");

    Gadget binaryOpGadgets[4];
    binaryOpGadgets[0] = createGadget(BINARY_OP,"sub eax, ecx");
    binaryOpGadgets[0] = createGadget(BINARY_OP,"add eax, 4");
    binaryOpGadgets[0] = createGadget(BINARY_OP,"add eax, 0x4");
    binaryOpGadgets[0] = createGadget(BINARY_OP,"add eax, ebx");
    return 0;
}
