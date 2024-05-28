// Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

#include <cheerp/memprof.h>
#include <vector>
#include <cassert>


void populateItems(std::vector<int> & items, int item)
{
	char * p = (char *)malloc(sizeof(char) * 100);
	CHEERP_MEMPROF_TAG tag = cheerpMemProfAnnotate("populateItems, it should not leak since we used a preallocated vector");
	items.push_back(item);
	free(p);
	if (cheerpMemProfUsed(tag) > 0)
	{
		//This should never happens in this case, but here you may handle the error / save the state
		cheerpMemProfLive(tag);
		assert(false);
	}
	cheerpMemProfClose(tag);
}

void populateItemsRandom(std::vector<int> & items, int item)
{
	CHEERP_MEMPROF_TAG tag = cheerpMemProfAnnotate("populateItemsRandom, it should not leak, but it does. Why?");
	items.push_back(item % (rand()+1));
	if (cheerpMemProfUsed(tag) > 0 && item > 0)
	{
		//This should never happens in this case, but here you may handle the error / save the state
		cheerpMemProfLive(tag);
		assert(false);
		//In this case it will be triggered by rand() that needs to allocate his internal state
	}
	cheerpMemProfClose(tag);
}

// webMain is the entry point for web applications written in Cheerp.
void webMain()
{
	const int N = 5;

	CHEERP_MEMPROF_TAG tag = cheerpMemProfAnnotate("main");
	std::vector <int> items;
	items.reserve(N*2);
	for (int i=0; i<N; i++)
	{
		populateItems(items, i*i);
	}
	for (int i=0; i<N; i++)
	{
		populateItemsRandom(items, i*i);
	}
	items.clear();
	cheerpMemProfClose(tag);
}
