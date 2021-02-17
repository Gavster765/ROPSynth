#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gadgets.h"
#include "utils.h"

Gadget createGadget(GadgetType type, const char* assembly){
    char* line = strdup(assembly);  // Make string writable
    char* opcode = strtok(line, " ");  // Peel off opcode
    char* operands = strtok(NULL, "");  // Save all operands
    char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
    int numOperands = getOperands(operandList, operands);

    Gadget gadget = {
        type,
        assembly,
        opcode,
        numOperands,
        operandList
        };
    return gadget;
}

Gadgets loadGadgets(){
    int numLoadConstGadgets = 8;
    char* loadConstGadgetsStrings[8] = {
        "pop eax",
        "pop ecx",
        "pop edx",
        "pop ebx",
        "pop esp",
        "pop ebp",
        "pop esi",
        "pop edi"
    };
    Gadget loadConstGadgets[numLoadConstGadgets];
    for(int i = 0 ; i < numLoadConstGadgets ; i++){
        loadConstGadgets[i] = createGadget(LOAD_CONST,loadConstGadgetsStrings[i]);   
    }

    int numBinaryOpGadgets = 4;
    char* binaryOpGadgetsStrings[4] = {
        "sub eax, ecx",
        "add eax, 4",
        "add eax, 0x4",
        "add eax, ebx"
    };
    Gadget binaryOpGadgets[numBinaryOpGadgets];
    for(int i = 0 ; i < numBinaryOpGadgets ; i++){
        binaryOpGadgets[i] = createGadget(BINARY_OP,binaryOpGadgetsStrings[i]);   
    }

    int numMoveRegGadgets = 3;
    char* moveRegGadgetsStrings[3] = {
        "mov eax, ecx",
        "mov eax, edx",
        "mov eax, ebx"
    };
    Gadget moveRegGadgets[numMoveRegGadgets];
    for(int i = 0 ; i < numMoveRegGadgets ; i++){
        moveRegGadgets[i] = createGadget(MOVE_REG,moveRegGadgetsStrings[i]);   
    }

    Gadgets gadgets = {
        numLoadConstGadgets,
        loadConstGadgets,
        numBinaryOpGadgets,
        binaryOpGadgets,
        numMoveRegGadgets,
        moveRegGadgets
    };

    return gadgets;
}
