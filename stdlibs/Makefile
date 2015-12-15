.PHONY: all install

INSTALL_PREFIX ?= /opt/cheerp
CHEERP_PREFIX ?= /opt/cheerp

all: libstdlibs.bc

libstdlibs.full.bc: ${CHEERP_PREFIX}/lib/libc.bc ${CHEERP_PREFIX}/lib/libm.bc ${CHEERP_PREFIX}/lib/libc++.bc ${CHEERP_PREFIX}/lib/libc++abi.bc
	${CHEERP_PREFIX}/bin/llvm-link $^ -o $@

libstdlibs.bc: libstdlibs.full.bc
	${CHEERP_PREFIX}/bin/opt -O3 -PreExecute $^ -o $@

install: libstdlibs.bc
	cp libstdlibs.bc ${INSTALL_PREFIX}/lib/libstdlibs.bc
