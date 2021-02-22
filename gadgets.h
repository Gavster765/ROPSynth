typedef enum {
    LOAD_CONST,
    MOVE_REG,
    ARITH_OP
} GadgetType;

typedef struct Gadget {
    GadgetType type;  // Type of gadget
    char* assembly;  // Actual string
    char* opcode;
    int numOperands;
    char** operands;
} Gadget;

typedef struct Gadgets {
    int numLoadConstGadgets;
    Gadget* loadConstGadgets;
    int numArithOpGadgets;
    Gadget* arithOpGadgets;
    int numMoveRegGadgets;
    Gadget* moveRegGadgets;
} Gadgets;

Gadget createGadget(GadgetType type, char* assembly);

Gadgets loadGadgets();