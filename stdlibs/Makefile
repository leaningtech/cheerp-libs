.PHONY: install

INSTALL_PREFIX ?= /opt/cheerp
CHEERP_PREFIX ?= /opt/cheerp

install:
	${CHEERP_PREFIX}/bin/llvm-link \
		${CHEERP_PREFIX}/lib/libc.bc \
		${CHEERP_PREFIX}/lib/libm.bc \
		${CHEERP_PREFIX}/lib/libc++.bc \
		${CHEERP_PREFIX}/lib/libc++abi.bc \
		-o ${INSTALL_PREFIX}/lib/libstdlibs.full.bc
	${CHEERP_PREFIX}/bin/opt \
		-O3 -PreExecute \
		-o ${INSTALL_PREFIX}/lib/libstdlibs.bc \
		${INSTALL_PREFIX}/lib/libstdlibs.full.bc
	rm ${INSTALL_PREFIX}/lib/libstdlibs.full.bc
