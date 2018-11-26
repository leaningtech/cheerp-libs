DIRECTORY := ${CURDIR}

CHEERP_CPP_COMPILER := /opt/cheerp/bin/clang++

CHEERP_C_COMPILER := /opt/cheerp/bin/clang

COMPILER_OPTIONS := -target cheerp -lmemprof -cheerp-mode=asmjs -cheerp-pretty-code

MEMPROFUIJS_LOCATION := ../build_asmjs/memprofui.js

LINK_NAME := memprofui.js

all: check_memprofui
	mkdir -p ${DIRECTORY}/run_tests
	${CHEERP_CPP_COMPILER} ${COMPILER_OPTIONS} test1.cpp -o ${DIRECTORY}/run_tests/test1.js
	${CHEERP_CPP_COMPILER} ${COMPILER_OPTIONS} test2.cpp -o ${DIRECTORY}/run_tests/test2.js
	${CHEERP_C_COMPILER} ${COMPILER_OPTIONS} test3.c -o ${DIRECTORY}/run_tests/test3.js
	${CHEERP_CPP_COMPILER} ${COMPILER_OPTIONS} -cheerp-linear-heap-size=2 memoryHoarder.cpp -o ${DIRECTORY}/run_tests/memoryHoarder.js
	node run_tests/test1.js > tmp
	test -s tmp
	node run_tests/test2.js > tmp
	test -s tmp
	node run_tests/test3.js > tmp
	test -s tmp
	chromium-browser example1.html
	chromium-browser example2.html
	chromium-browser example3.html
	chromium-browser exampleUI1.html
	chromium-browser exampleUI2.html

link_memprofui:
	test -e ${MEMPROFUIJS_LOCATION} && ln -s ${MEMPROFUIJS_LOCATION} ${LINK_NAME}

check_memprofui:
	if test -L ${LINK_NAME}; then true ; else echo "There should be ${LINK_NAME} link in this folder, possibly make link_memprofui may solve it" && false; fi
	if test -e ${LINK_NAME}; then true ; else echo "${LINK_NAME} exist but do not actlually link to anything" && false; fi
	
clean:
	rm -rf run_tests
	rm -f tmp
