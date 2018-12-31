.PHONY: install

INSTALL_PREFIX ?= /opt/cheerp
CHEERP_PREFIX ?= /opt/cheerp

all: libwasm.bc

libwasm.bc: memory.bc
	${CHEERP_PREFIX}/bin/llvm-link -o $@ $^

%.bc: %.cpp
	${CHEERP_PREFIX}/bin/clang++ -c $^ -o $@ -target cheerp -cheerp-mode=wasm

install_lib: libwasm.bc
	mkdir -p ${INSTALL_PREFIX}/lib/
	cp -v $^ ${INSTALL_PREFIX}/lib/

install: install_lib

clean:
	rm -fv libwasm.bc memory.bc
