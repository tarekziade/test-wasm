.PHONY: all clean

TARGET = test_builtins
SRC = main.cpp
OUT_JS = $(TARGET).js
OUT_WASM = $(TARGET).wasm
PRE_JS = pre.js

all:
	emcc $(SRC) -o $(OUT_JS) -sUSE_PTHREADS=1 -sWASM=1 -sALLOW_MEMORY_GROWTH=0 -sMODULARIZE -sEXPORT_ES6 -sMAIN_MODULE=1 -sEXPORTED_FUNCTIONS=_testWasmBuiltin -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -sERROR_ON_UNDEFINED_SYMBOLS=0 --pre-js $(PRE_JS)

cpp:
	g++ $(SRC) -o test_cpp -std=c++11

clean:
	rm -f $(OUT_JS) $(OUT_WASM)
