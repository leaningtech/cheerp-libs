#include <cheerp/memprof.h>
#include <stdlib.h>

char *allocateArray(int dim)
{
	CHEERP_MEMPROF_TAG x = cheerpMemProfAnnotate("function");
	cheerpMemProfClose(x);
	return (char *)malloc(dim);
}

//Works even for C sources
int main()
{
	const int N = 200;
	CHEERP_MEMPROF_TAG x = cheerpMemProfAnnotate("main");
	char *p = allocateArray(N);
	for (int i=1; i<N; i++) p[i] += p[i-1];
	cheerpMemProfClose(x);
	free(p);
	return 0;
}
