#ifndef GADGET_H
#define GADGET_H

typedef enum {
    LOAD_CONST,
    MOVE_REG,
    ARITH_OP,
    STORE_MEM,
    LOAD_MEM,
    SPECIAL,
    // Pseudo only
    COPY,
    COMP,
    END,
    JUMP
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

    int numSpecialGadgets;
    Gadget* specialGadgets;

    int* numSynthComps;
    SynthComp* synthComps;

    int* numJumps;
    int* jumpAddrs;
} Gadgets;

Gadget createGadget(GadgetType type, char* assembly);

Gadgets loadGadgets();
void freeGadgets(Gadgets gadgets);

void addSynthComp(char* spec, char* synth, Gadgets gadgets);
char* getSynth(char* spec, Gadgets gadgets);

void addJumpAddr(int addr, Gadgets gadgets);
int checkJump(int line, Gadgets gadgets); 

#endif /* GADGET_H */
