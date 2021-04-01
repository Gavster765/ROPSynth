Test if else blocks

    "Var x 1",
    "Const y 2",
    "Const z 3",

    "If x > z",
        "Sub x z",
    "ElseIf x < z",
        "Add x y",
    "Else",
        "Add x z",
    "End"

Loop test
    
    "Var x 3",
    "Const y 1",
    "Const z 0",

    "While x > z",
        "Sub x y",
    "End"

Basic synthesis test

    "Var x 2",
    "Const y 3",

    "Mul x y"

Synthesis with 'for' loop

    "Var x 3",
    "Const y 2",

    "Var i 0",
    "Const end 3",
    "Const one 1",
    
    "While i <= end",
        "Add x y",
        "Add i one",
    "End"