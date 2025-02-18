.PHONY: all clean

TARGET = test_builtins
SRC = main.cpp
OUT_JS = $(TARGET).js
OUT_WASM = $(TARGET).wasm

all:
	emcc $(SRC) -o $(OUT_JS) -sEXPORTED_FUNCTIONS=_testWasmBuiltin -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -sERROR_ON_UNDEFINED_SYMBOLS=0

clean:
	rm -f $(OUT_JS) $(OUT_WASM)
