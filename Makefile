all:
	clang utils.c gadgets.c main.c -o main

debug:
	clang -g -O0 utils.c gadgets.c main.c -o main