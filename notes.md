Issues:
• Variables can get stuck <- i.e moved to rdx but no gadgets to move them out of rdx
• May need to look ahead a little <- on fail go back and try again?? <- some kind of tree?
• Add x x doesn't work

Report notes:
mention stale vars
gadgets that end in jumps not return? (jop)
talk about fresh variables

TODO?:
• Extend language to allow output via a syscall gadget
• Try to use some kind of backtracking to look for other solutions if the first fails
• Consider multi instruction gadget? E.g. gadgets that clobber other registers?
• Vary number of components given to cegis
• Allow more operations in cegis api
• Double moves?

DONE:
• Extend language to include basic loops using jump gadgets
• Consider trying to us memory to save variable values and allow more to be saved
• Try to use an SMT solver to search for a replacement if, e.g. no add gadget is found
• Update old vars register locations after cegis
• Reason for occasional error is the order of arguments received from cegis
• Fail nicely if cegis fails
• Store results <- don't need to redo long synthesis in a loop
• Make sure to store and load before/after loops/ifs (otherwise var location not known)
• When implementing loops - consider variable lifespan?
• Lifespan issues -> in new createPseudo the lifespan of old vars updated? <- used tmpVars but need to update now
• Need to update lifespan for unused alt vars if alt longer than prog