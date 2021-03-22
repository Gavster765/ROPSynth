Test if else blocks

    "Var x 1",
    "Var y 2",
    "Add x y",
    "Var z 3",

    "If x > z",
        "Sub x z",
    "ElseIf x < z",
        "Add x y",
    "Else",
        "Add x z",
    "End"

Loop test
    
    "Var x 3",
    "Var y 1",
    "Var z 0",

    "While x > z",
        "Sub x y",
    "End"

Basic synthesis test

    "Var x 2",
    "Var y 3",

    "Mul x y"