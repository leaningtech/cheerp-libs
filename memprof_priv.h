// Copyright 2024 Leaning Technologies Ltd. All Rights Reserved.
#ifndef CHEERP_MEMPROF_PRIV
#define CHEERP_MEMPROF_PRIV

#include <cheerp/client.h>
#include <sys/types.h>

class [[cheerp::jsexport]] [[cheerp::genericjs]] CheerpMemProf
{
private:
public:
	CheerpMemProf()
	{
	}
	client::TArray<client::Object*>* liveAllocations();
	client::Object* liveAllocationsTree(bool isTopDown);
	int32_t totalLiveMemory();
};


#endif
