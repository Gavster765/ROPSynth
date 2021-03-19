CLANG_BIN ?= $(shell which clang)
CARGO_BIN ?= $(shell which cargo)

run: build
	./rust_test
run_clean: clean build
	./rust_test
clean:
	$(CARGO_BIN) clean
	rm -f ./rust_test
build:
	$(CARGO_BIN) build --release
	$(CLANG_BIN) -o ./rust_test ./rust_test.c -Isrc  -L. -l:target/release/librop_compiler.so