Issues:
Variables can get stuck <- i.e moved to rdx but no gadgets to move them out of rdx
May need to look ahead a little <- on fail go back and try again?? <- some kind of tree?
When implementing loops - consider variable lifespan?

Report notes:
mention stale vars
talk about fresh variables

TODO?:
• Extend language to include basic loops using jump gadgets
• Extend language to allow output via a syscall gadget
• Consider trying to us memory to save variable values and allow more to be saved
• Try to use some kind of backtracking to look for other solutions if the first fails
• Try to use an SMT solver to search for a replacement if, e.g. no add gadget is found
• Consider multi instruction gadget? E.g. gadgets that clobber other registers?