#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gadgets.h"
#include "utils.h"

// Create a new gadget given a type and the asm string
Gadget createGadget(GadgetType type, char* assembly){
    char* assm = strdup(assembly);  // Make string writable
    char* line = strdup(assembly);
    char* opcode = strtok(line, " ");  // Peel off opcode
    char* operands = strtok(NULL, "");  // Save all operands
    char** operandList = malloc(3*20*sizeof(char));  // Max 3 operands at 20 chars each
    int numOperands = getGadgetOperands(operandList, operands);

    Gadget gadget = {
        type,
        assm,
        opcode,
        numOperands,
        operandList
        };
    return gadget;
}

// Read gadgets text file into Gadgets structure
Gadgets loadGadgets(){
    int numLoadConstGadgets, numArithOpGadgets, numMoveRegGadgets, 
        numStoreMemGadgets, numLoadMemGadgets;
    const int max = 30;
    char line[max];
    FILE *f = fopen("gadgets.txt", "r");
    
    fscanf(f, "%d,%d,%d,%d,%d\n",&numLoadConstGadgets, &numArithOpGadgets, 
        &numMoveRegGadgets, &numStoreMemGadgets, &numLoadMemGadgets);
    Gadget* loadConstGadgets = malloc(sizeof(Gadget) * numLoadConstGadgets);
    Gadget* arithOpGadgets = malloc(sizeof(Gadget) * numArithOpGadgets);
    Gadget* moveRegGadgets = malloc(sizeof(Gadget) * numMoveRegGadgets);
    Gadget* storeMemGadgets = malloc(sizeof(Gadget) * numStoreMemGadgets);
    Gadget* loadMemGadgets = malloc(sizeof(Gadget) * numLoadMemGadgets);
    SynthComp* synthComps = malloc(sizeof(SynthComp) * 10);  // Max 10 saved synthesis components
    int* numSynthComps = malloc(sizeof(int));
    *numSynthComps = 0;

    Gadget* curr;
    int count;
    GadgetType type;
    while (!feof(f)){
        fgets(line, max , f);
        line[strcspn(line, "\n")] = '\0';

        if(strcmp(line,"") == 0){
            continue;
        }
        else if(strcmp(line,"loadConst") == 0){
            curr = loadConstGadgets;
            type = LOAD_CONST;
            count = 0;
        }
        else if(strcmp(line,"arithOp") == 0){
            curr = arithOpGadgets;
            type = ARITH_OP;
            count = 0;
        }
        else if(strcmp(line,"moveReg") == 0){
            curr = moveRegGadgets;
            type = MOVE_REG;
            count = 0;
        }
        else if(strcmp(line,"storeMem") == 0){
            curr = storeMemGadgets;
            type = STORE_MEM;
            count = 0;
        }
        else if(strcmp(line,"loadMem") == 0){
            curr = loadMemGadgets;
            type = LOAD_MEM;
            count = 0;
        }
        else {
            curr[count] = createGadget(type, line);
            count++;
        }
    }
    fclose(f);

    Gadgets gadgets = {
        numLoadConstGadgets,
        loadConstGadgets,
        numArithOpGadgets,
        arithOpGadgets,
        numMoveRegGadgets,
        moveRegGadgets,
        numStoreMemGadgets,
        storeMemGadgets,
        numLoadMemGadgets,
        loadMemGadgets,
        numSynthComps,  // 0 components already synthesized
        synthComps
    };

    return gadgets;
}

void addSynthComp(char* spec, char* synth, Gadgets gadgets) {
    SynthComp s = {
        .spec = spec,
        .synth = synth
    };
    gadgets.synthComps[*gadgets.numSynthComps] = s;
    (*gadgets.numSynthComps)++;
}

char* getSynth(char* spec, Gadgets gadgets) {
    for (int i = 0 ; i < *gadgets.numSynthComps ; i++) {
        if (strcmp(gadgets.synthComps[i].spec, spec) == 0) {
            return gadgets.synthComps[i].synth;
        }
    }
    return NULL;
}
