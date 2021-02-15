typedef enum {
    LOAD_CONST,
    MOVE_REG,
    BINARY_OP
} GadgetType;

typedef struct Gadget {
    GadgetType type;  // Type of gadget
    const char* assembly;  // Actual string
    char* opcode;
    int numOperands;
    char** operands;
} Gadget;

typedef struct Gadgets {
    int numLoadConstGadgets;
    Gadget* loadConstGadgets;
    int numBinaryOpGadgets;
    Gadget* binaryOpGadgets;
    int numMoveRegGadgets;
    Gadget* moveRegGadgets;
} Gadgets;

Gadget createGadget(GadgetType type, const char* assembly);

Gadgets loadGadgets();