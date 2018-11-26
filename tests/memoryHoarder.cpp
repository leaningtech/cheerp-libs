//#include <cheerp/clientlib.h>
#include <cheerp/client.h>
#include <cheerp/memprof.h>
#include <vector>


int sort_of_random()
{
	return rand()%100000;
}

void function()
{
	static std::vector<char *> M;
	static int times = 0;
	int DIM = sort_of_random();
	char * P = (char *)malloc(DIM);
	if (M.size() > 15)
	{
		for (int i=0; i<M.size(); i++) free(M[i]);
		M.clear();
		times=sort_of_random()/20;
	}
	if (M.size() > sort_of_random()%(times++/100))
	{
		int index = sort_of_random()%M.size();
		free(M[index]);
		M[index] = P;
	}
	else
		M.push_back(P);
	for (int i = 6; i<DIM; i++) P[i] +=P[i-1] + P[i-5];
}

[[cheerp::genericjs]] static void rafHandler()
{
	function();
	client::requestAnimationFrame(cheerp::Callback(rafHandler));
}


[[cheerp::genericjs]] void webMain()
{
	client::requestAnimationFrame(cheerp::Callback(rafHandler));
}
