# ROP Synthesizer
Final year project - ROP compiler

## Dependencies
`sudo apt install clang`

`sudo apt install cargo`

`sudo apt install libz3-dev`

## Build
Can build by using `make`

A sample program is available in `prog.txt` this program can be ran using `make run`

Example programs can be found in example/programs and the results of running them in examples/results

All examples can be ran with `make example`

## Language
Program files must end with a new line character

Variables:

`Var <var> <value>`

`Const <var> <value>`

`Copy <var> <var/value>`

Comparison Operators:

`= < <= > >= !=`

`<comp> And <comp>`

Operations:

`Add <var> <var>`

`Sub <var> <var>`

`Mul <var> <var>`

`Div <var> <var>`

`Mod <var> <var>`

`Xor <var> <var>`

`And <var> <var>`

`Neg <var>`

`Not <var>`

Conditionals:

```
If <var> <comp op> <var> 
    code
ElseIf <var> <comp op> <var>
    code
Else
    code
End
```

Loops:
```
While <var> <comp op> <var>
    code
End

Break
```

Input/Output:

Both operations operate on only a single byte from stdin/stdout respectively
```
Read <var> 

Write <var>
```

## References

Code used to find alternative gadgets in 'synth-loop-free-prog' taken from: https://github.com/fitzgen/synth-loop-free-prog
