build:
	clang utils.c gadgets.c main.c var.c -o main

run: build
	./main

debug:
	clang -g -O0 utils.c gadgets.c var.c main.c -o main