EXAMPLES:= fib fizzbuzz jump cegis read_write if_else while

build:
	cargo build --release --manifest-path=synth-loop-free-prog/Cargo.toml
	clang utils.c gadgets.c var.c synth-loop-free-prog/synthesis.c main.c -o main -I. -Isrc synth-loop-free-prog/target/release/librop_compiler.so -Wl,-rpath,synth-loop-free-prog/target/release -Wall -pedantic -fsanitize=undefined -fsanitize=address
run: build
	./main prog.txt
debug:
	cargo build --release --manifest-path=synth-loop-free-prog/Cargo.toml
	clang -g -O0 utils.c gadgets.c var.c synth-loop-free-prog/synthesis.c main.c -o main -I. -Isrc synth-loop-free-prog/target/release/librop_compiler.so -Wl,-rpath,synth-loop-free-prog/target/release -Wall -pedantic -fsanitize=undefined -fsanitize=address

example: build
	for file in $(EXAMPLES); do\
		./main examples/programs/$$file.txt > examples/results/$$file.txt ;\
	done
