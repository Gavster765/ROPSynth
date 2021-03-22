build:
	cargo build --release --manifest-path=synth-loop-free-prog/Cargo.toml
	clang utils.c gadgets.c var.c synth-loop-free-prog/synthesis.c main.c -o main -I. -Isrc  -L. -l:synth-loop-free-prog/target/release/librop_compiler.so
run: build
	./main
debug:
	cargo build --release --manifest-path=synth-loop-free-prog/Cargo.toml
	clang -g -O0 utils.c gadgets.c var.c synth-loop-free-prog/synthesis.c main.c -o main -I. -Isrc  -L. -l:synth-loop-free-prog/target/release/librop_compiler.so