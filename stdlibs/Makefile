.PHONY: all install

INSTALL_PREFIX ?= /opt/cheerp
CHEERP_PREFIX ?= /opt/cheerp

all: genericjs asmjs

genericjs: genericjs/libstdlibs.bc

asmjs: asmjs/libstdlibs.bc

genericjs/libstdlibs.full.bc: ${CHEERP_PREFIX}/lib/genericjs/libc.bc ${CHEERP_PREFIX}/lib/genericjs/libm.bc ${CHEERP_PREFIX}/lib/genericjs/libc++.bc ${CHEERP_PREFIX}/lib/genericjs/libc++abi.bc
	@mkdir -p genericjs
	${CHEERP_PREFIX}/bin/llvm-link $^ -o $@

genericjs/libstdlibs.bc: genericjs/libstdlibs.full.bc
	${CHEERP_PREFIX}/bin/opt -passes="default<O3>,PreExecute" $^ -o $@

asmjs/libstdlibs.full.bc: ${CHEERP_PREFIX}/lib/asmjs/libc.bc ${CHEERP_PREFIX}/lib/asmjs/libm.bc ${CHEERP_PREFIX}/lib/asmjs/libc++.bc ${CHEERP_PREFIX}/lib/asmjs/libc++abi.bc
	@mkdir -p asmjs
	${CHEERP_PREFIX}/bin/llvm-link $^ -o $@

asmjs/libstdlibs.bc: asmjs/libstdlibs.full.bc
	${CHEERP_PREFIX}/bin/opt -passes="default<O3>,PreExecute" $^ -o $@

install_asmjs: asmjs
	cp asmjs/libstdlibs.bc ${INSTALL_PREFIX}/lib/asmjs/libstdlibs.bc

install_genericjs: genericjs
	cp genericjs/libstdlibs.bc ${INSTALL_PREFIX}/lib/genericjs/libstdlibs.bc

install: install_genericjs install_asmjs
