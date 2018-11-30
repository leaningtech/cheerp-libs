// Copyright 2018 Leaning Technologies Ltd. All Rights Reserved. 

#include <cheerp/memprof.h>
#include <map>
#include <vector>

int *leak(int N)
{
	cheerpMemProfAnnotate("leak");
	int *p = (int *)malloc(sizeof(int) * N);
	for (int i=1; i<N; i++) p[i] += p[i-1];
	return p;
}

int *leakOnlyUntilThereIsSpace(int MAX, int K = 1)
{
	int *p = leak(K);
	if (cheerpMemProfTotalUsed() < MAX)
	{
		p = leakOnlyUntilThereIsSpace(MAX, K*10);
		p = leakOnlyUntilThereIsSpace(MAX, K*10);
	}
	return p;
}

void functionA(int N)
{
	CHEERP_MEMPROF_TAG tag = cheerpMemProfAnnotate("functionA");
	int *fibonacci = (int *)malloc (sizeof(int)*1000);
	for (int i=0; i<1000; i++)
	{
		fibonacci[i] = 1;
		if (i > 1) fibonacci[i] = fibonacci[i-1] + fibonacci[i-2];
	}
	leak(fibonacci[N]);
	free(fibonacci);
	cheerpMemProfClose(tag);
}

// A program that do not do anything meaningful apart from allocating memory without ever calling free.
// The calls to the stack trace allow to identify the function that leaks

void webMain()
{
	CHEERP_MEMPROF_TAG tag = cheerpMemProfAnnotate("main");
	functionA(10);
	functionA(13);
	cheerpMemProfLive(tag);
	leakOnlyUntilThereIsSpace(100000);
	cheerpMemProfClose(tag);
}
