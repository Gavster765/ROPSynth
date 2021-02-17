typedef enum {
    LOAD_CONST,
    MOVE_REG,
    BINARY_OP
} GadgetType;

// typedef struct LoadConst {
//     char* out;
//     int value;
// } LoadConst;

// typedef struct BinOp {
//     char* out;
//     char opcode;
//     char* operand1;
//     char* operand2;
// } BinOp;

// // Ect as intermediate step - use switch on type?
// typedef struct Type {
//     GadgetType type;  // Type of gadget
//     LoadConst loadConst;
//     BinOp binOp;
// } Type;

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