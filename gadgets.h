#ifndef GADGET_H
#define GADGET_H

typedef enum {
    LOAD_CONST,
    MOVE_REG,
    ARITH_OP,
    STORE_MEM,
    LOAD_MEM,
    COPY, // TODO - move as only a pseudo type??
    COMP, // TODO - move as only a pseudo type??
    END // TODO - move as only a pseudo type??
} GadgetType;

typedef struct Gadget {
    GadgetType type;  // Type of gadget
    char* assembly;  // Actual string
    char* opcode;
    int numOperands;
    char** operands;
} Gadget;

typedef struct SynthComp {
    char* spec;
    char* synth;
} SynthComp;

typedef struct Gadgets {
    int numLoadConstGadgets;
    Gadget* loadConstGadgets;

    int numArithOpGadgets;
    Gadget* arithOpGadgets;

    int numMoveRegGadgets;
    Gadget* moveRegGadgets;

    int numStoreMemGadgets;
    Gadget* storeMemGadgets;
    
    int numLoadMemGadgets;
    Gadget* loadMemGadgets;

    int* numSynthComps;
    SynthComp* synthComps;
} Gadgets;

Gadget createGadget(GadgetType type, char* assembly);

Gadgets loadGadgets();

void addSynthComp(char* spec, char* synth, Gadgets gadgets);
char* getSynth(char* spec, Gadgets gadgets);

#endif /* GADGET_H */