.PHONY: install

INSTALL_PREFIX ?= /opt/cheerp
CHEERP_PREFIX ?= /opt/cheerp

all: libGLESv2.bc

libGLESv2.bc: webgles.bc webglesutils.bc
	${CHEERP_PREFIX}/bin/llvm-link -o $@ $^

%.bc: %.cpp
	${CHEERP_PREFIX}/bin/clang++ ${CXXFLAGS} -c $^ -o $@ -target cheerp -O3

install_headers: webgles.h gl2.h
	mkdir -p ${INSTALL_PREFIX}/include/GLES2/
	cp -v $^ ${INSTALL_PREFIX}/include/GLES2/

install_lib: libGLESv2.bc
	mkdir -p ${INSTALL_PREFIX}/lib/
	cp -v $^ ${INSTALL_PREFIX}/lib/

install: install_headers install_lib

clean:
	rm -fv libGLESv2.bc webgles.bc webglesutils.bc
