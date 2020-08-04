// Copyright 2018 Leaning Technologies Ltd. All Rights Reserved.

#include <cheerp/client.h>
#include <cheerp/types.h>
#include "memprof.h"

//Undefine the next row if you prefer to have the stacktrace built at the error generation
#define STACKTRACE_BUILT_LAZILY

//Change the number here to have a different depth, or comment it to have it unlimited
//#define STACKTRACE_DEPTH 15

extern "C"
{
	// We redefine internal symbols here
	__attribute__((cheerp_wasm)) void* _malloc_r(void* reent, size_t size);
	__attribute__((cheerp_wasm)) void* _realloc_r(void* reent, void* ptr, size_t size);
	__attribute__((cheerp_wasm)) void* _calloc_r(void* reent, size_t nmemb, size_t size);
	void __attribute__((cheerp_wasm)) _free_r(void* reent, void* ptr);
	extern __attribute__((cheerp_wasm)) void* mALLOc(void* reent, size_t size);
	extern __attribute__((cheerp_wasm)) void* rEALLOc(void* reent, void* ptr, size_t size);
	extern __attribute__((cheerp_wasm)) void* cALLOc(void* reent, size_t nmemb, size_t size);
	extern __attribute__((cheerp_wasm)) void fREe(void* reent, void* ptr);
}

class CheerpAllocationsTracker
{
	class AllocationData
	{
	public:
		AllocationData(const size_t dim, client::Error* error) :
			AllocationData(dim, error, NULL) {}
		AllocationData(const size_t dim, client::String* stackTrace) :
			AllocationData(dim, NULL, stackTrace) {}
		client::String* getStackTrace()
		{
			if (stackTrace == NULL)
			{
				stackTrace = error->get_stack();
				if (stackTrace->startsWith("Error"))
					stackTrace = stackTrace->slice(6);
				error = NULL;
			}
			return stackTrace;
		}
		client::TArray<client::String>* getPrettyStackTrace(bool isTopDown)
		{
			auto& curr = arrayStackTrace[isTopDown?1:0];
			if (curr == NULL)
			{
				if (arrayStackTrace[0] == NULL)
				{
					StackTraceParsing stackTraceParsing(getStackTrace());
					arrayStackTrace[0] = stackTraceParsing.getStackTrace();
				}
				if (isTopDown)
				{
					arrayStackTrace[1] = new client::TArray<client::String>;
					for (int i = 0; i<arrayStackTrace[0]->get_length(); i++)
						arrayStackTrace[1]->push((*arrayStackTrace[0])[i]);
					arrayStackTrace[1]->reverse();
				}
			}
			return curr;
		}
		const CHEERP_MEMPROF_TAG insertionTime;
		const size_t dim;
	private:
		AllocationData(const size_t dim, client::Error* error, client::String* stackTrace) :
			insertionTime(getTimestamp()), dim(dim),
			error(error), stackTrace(stackTrace), arrayStackTrace{NULL,NULL} {}
		client::Error* error;
		client::String* stackTrace;
		client::TArray<client::String>* arrayStackTrace[2];
	};
	class StatTreeNode
	{
	public:
		StatTreeNode(client::String* name) :
			liveMemory(0), function(name), childrenNodes(NULL) {}
		void addAllocation(const bool isTopDown, AllocationData* data, int depth)
		{
			liveMemory += data->dim;

			auto stack = data->getPrettyStackTrace(isTopDown);
			if (depth >= stack->get_length())
				return;
			if (childrenNodes == NULL)
				childrenNodes = new client::TMap<client::String*, StatTreeNode*>;

			client::String* line = new client::String((*stack)[depth]);
			if (childrenNodes->has(line) == false)
			{
				childrenNodes->set(line, new StatTreeNode(line));
			}
			childrenNodes->get(line)->addAllocation(isTopDown, data, depth+1);
		}
		client::Object* buildObject()
		{
			client::TArray<client::Object*>* children = new client::TArray<client::Object*>;
			if (childrenNodes)
			{
				client::TArray<StatTreeNode>* sons= new client::TArray<StatTreeNode>;

				childrenNodes->forEach(cheerp::Callback([&sons](StatTreeNode* a, client::String* s)
							{
							sons->push(a);
							}
							));

				sons -> sort(cheerp::Callback([](StatTreeNode* a, StatTreeNode* b)
							{
							if (a->liveMemory < b->liveMemory)
								return 1;
							if (a->liveMemory > b->liveMemory)
								return -1;
							return 0;
							}
							));
				sons -> forEach(cheerp::Callback([&children](StatTreeNode* a)
							{
							children->push(a->buildObject());
							}
							));
			}
			return CHEERP_OBJECT(liveMemory, function, children);
		}
	private:
		size_t liveMemory;
		client::String* function;
		client::TMap<client::String*,StatTreeNode*>* childrenNodes;
	};
	CheerpAllocationsTracker()
	{
		nameTags = new client::TMap<CHEERP_MEMPROF_TAG, client::String*>;
		allocatedMemory = new client::TMap<uintptr_t, AllocationData*>;
		totalMemoryAllocated = 0;
	}
	static CheerpAllocationsTracker *instance()
	{
		static CheerpAllocationsTracker *AT = 0;
		if (AT == 0)
			AT = new CheerpAllocationsTracker();
		return AT;
	}
	void registerAlloc_(uintptr_t ptr, size_t dim, client::String* s)
	{
		if (ptr == 0)
			return;
		allocatedMemory->set(ptr, new AllocationData(dim, s));
		totalMemoryAllocated += dim;
	}
	void registerAlloc_(uintptr_t ptr, size_t dim, client::Error* e)
	{
		if (ptr == 0)
			return;
		allocatedMemory->set(ptr, new AllocationData(dim, e));
		totalMemoryAllocated += dim;
	}
	void registerFree_(uintptr_t ptr)
	{
		if (ptr == 0)
			return;
		if (allocatedMemory->has(ptr) == false)
			client::console.log("Invalid free detected\n", getStackTrace());
		else
		{
			totalMemoryAllocated -= allocatedMemory->get(ptr)->dim;
			allocatedMemory->delete_(ptr);
		}
	}
	CHEERP_MEMPROF_TAG createLabel_(client::String* name_tag)
	{
		timestampManager.increaseTimestamp();
		CHEERP_MEMPROF_TAG t = getTimestamp();
		nameTags->set(t, name_tag);
		return t;
	}
	void statusTracking_(CHEERP_MEMPROF_TAG tag)
	{
		client::String* s = new client::String("");
		if (nameTags->get(tag)->get_length() != 0)
			s = s->concat("Tag ")->concat(nameTags->get(tag))->concat("\n");
		else
			s = s->concat("Untagged\n");
		s = s->concat("Memory allocated after tag : ")->concat(client::String(memoryUsedStartingAtTag(tag)).padStart(8))->concat("\n");
		s = s->concat("Memory allocated in total  : ")->concat(client::String(totalUsed()).padStart(8));
		client::console.log(s);
		allocatedMemory->forEach(cheerp::Callback([tag](AllocationData *d, uintptr_t ptr)
			{
			if (d->insertionTime < tag)
				return;
			logMemoryBlock(d, ptr);
			}
			));
	}
	size_t memoryUsedStartingAtTag(CHEERP_MEMPROF_TAG tag)
	{
		size_t currMemoryAllocated = 0;
		allocatedMemory->forEach(cheerp::Callback([tag, &currMemoryAllocated](AllocationData *d, uintptr_t ptr)
			{
			if (d->insertionTime < tag)
				return;
			currMemoryAllocated += d->dim;
			}
			));
		return currMemoryAllocated;
	}
	size_t totalUsed()
	{
		return totalMemoryAllocated;
	}
	static client::String *getStackTrace()
	{
		client::Error* e = getError();
		client::String* s = e->get_stack();
		if (s->startsWith("Error"))
			s = s->slice(6);
		return s;
	}
	static client::Error *getError()
	{
		client::Object* old;
		__asm__("Error.stackTraceLimit" : "=r"(old) : );
#ifndef STACKTRACE_DEPTH
		__asm__("Error.stackTraceLimit = Infinity");
#else
		__asm__("Error.stackTraceLimit = %0" : : "r"(STACKTRACE_DEPTH + 5));
#endif
		client::Error* e = new client::Error();
		__asm__("Error.stackTraceLimit = %0" : : "r"(old));
		return e;
	}
public:
	static client::TArray<client::Object>* liveAllocations()
	{
		client::TArray<client::Object>* allocations= new client::TArray<client::Object>;

		instance()->allocatedMemory->forEach(cheerp::Callback([&allocations](AllocationData *d, uintptr_t ptr)
			{
			size_t size = d->dim;
			uintptr_t address = ptr;
			client::TArray<client::String>* stackTrace = d->getPrettyStackTrace(true);
			client::Object* composite = CHEERP_OBJECT(size, address, stackTrace);
			allocations->push(composite);
			}
			));
		return allocations;
	}
	static client::Object* liveAllocationsTree(const bool isTopDown)
	{
		StatTreeNode stat(new client::String(""));
		instance()->allocatedMemory->forEach(cheerp::Callback([&stat, isTopDown](AllocationData *d, uintptr_t ptr)
			{
			stat.addAllocation(isTopDown, d, 0);
			}
			));
		return stat.buildObject();
	}
	static void statusTracking(int tracker_descriptor)
	{
		CheerpAllocationsTracker::instance()->statusTracking_(tracker_descriptor);
	}

	static CHEERP_MEMPROF_TAG createLabel(const char *name_tag)
	{
		client::String* s = new client::String(name_tag);
		return CheerpAllocationsTracker::instance()->createLabel_(s);
	}

	static void closeTracking(CHEERP_MEMPROF_TAG tag)
	{
	}

	static size_t totalLiveMemory()
	{
		return CheerpAllocationsTracker::instance()->totalUsed();
	}

	static size_t liveMemory(CHEERP_MEMPROF_TAG tag)
	{
		return CheerpAllocationsTracker::instance()->memoryUsedStartingAtTag(tag);
	}

	static void registerAlloc(uintptr_t ptr, uintptr_t size)
	{
#ifndef STACKTRACE_BUILT_LAZILY
		client::String* x = CheerpAllocationsTracker::getStackTrace();
#else
		client::Error* x = CheerpAllocationsTracker::getError();
#endif
		CheerpAllocationsTracker::instance()->registerAlloc_(ptr, size, x);
	}

	static void registerFree(uintptr_t ptr)
	{
		CheerpAllocationsTracker::instance()->registerFree_(ptr);
	}

private:
	static uintptr_t charCode(const uintptr_t digit)
	{
		if (digit < 10)
			return (digit + '0');
		else
			return (digit - 10 + 'a');
	}
	static client::String *integerBaseToString(uintptr_t ptr, int base, int requiredDigits)
	{
		client::String *s = new client::String("");
		while (requiredDigits-- > 0 || ptr > 0)
		{
			s = client::String::fromCharCode(charCode(ptr%base))->concat(s);
			ptr /= base;
		}
		return s;
	}
	class TimestampManager
	{
	public:
		TimestampManager() : t(0) {}
		void increaseTimestamp()
		{
			t++;
		}
		CHEERP_MEMPROF_TAG getCurrentTimestamp()
		{
			return t;
		}
	private:
		CHEERP_MEMPROF_TAG t;
	};
	static CHEERP_MEMPROF_TAG getTimestamp()
	{
		return timestampManager.getCurrentTimestamp();
	}
	class StackTraceParsing
	{
	public:
		StackTraceParsing(client::String *s) : linesParsed(1), representation(s), parsed(false)
		{
			parse();
		}
		client::String* getRepresentation()
		{
			return representation;
		}
		client::TArray<client::String> *getStackTrace()
		{
			return stackTrace;
		}
	private:
		client::String *parseLine(const client::String *str, const client::String *separator, const int blocks, const int index)
		{
			client::TArray<client::String> *B = str->split(separator);
			if (B->get_length() != blocks || index >= blocks)
				return new client::String("");
			return (*B)[index];
		}
		void parseStackTrace(const client::String *separator, const int blocks, const int index)
		{
			client::TArray<client::String> *A = representation->split("\n");
			for (int i=0; i<A->get_length(); i++)
			{
				(*A)[i] = parseLine((*A)[i], separator, blocks, index);
			}
			int score = 0;
			for (int i=0; i<A->get_length() && (*A)[i]->get_length(); i++)
			{
				score++;
			}
			if (score > linesParsed)
			{
				linesParsed = score;
				while (A->get_length() > linesParsed)
				{
					A->pop();
				}
				stackTrace = A;
				parsed = true;
			}
		}
		bool isParsed()
		{
			return parsed;
		}
		void parse()
		{
			//Chrome-like stack trace
			parseStackTrace(client::String::fromCharCode(' '), 7, 5);

			//Firefox-like stack trace
			parseStackTrace(client::String::fromCharCode('@'), 2, 0);

			if (!isParsed())
				return;
			int mallocRow = -1;
			for (int i = 0; i<stackTrace->get_length(); i++)
			{
				if (((*stackTrace)[i])->indexOf("_malloc") >= 0)
				{
					mallocRow = i;
					break;
				}
			}
			stackTrace->splice(0, mallocRow + 1);

			representation = new client::String("");
			if (mallocRow == -1)
				representation = new client::String("\nRecommended: flag -cheerp-pretty-code");
			for (int i = 0; i<stackTrace->get_length(); i++)
			{
				representation = representation->concat("\n@")->concat(*(*stackTrace)[i]);
			}
			representation = representation->substring(1);
		}
	private:
		client::String *representation;
		client::TArray<client::String> *stackTrace;
		int linesParsed;
		bool parsed;
	};

	static client::String *prettyStack(client::String *s)
	{
		StackTraceParsing stackTraceParsing(s);
		return stackTraceParsing.getRepresentation();
	}
	static void logMemoryBlock(AllocationData *d, uintptr_t p)
	{
		client::String *s = new client::String("Address : ");
		s = s->concat(integerBaseToString(p, 16, 8));
		s = s->concat("\t\tSize : ");
		s = s->concat(integerBaseToString(d->dim, 10, 1)->padStart(8));
		s = s->concat("\n");
		s = s->concat(prettyStack(d->getStackTrace()));
		client::console.log(s);
	}
	client::TMap <CHEERP_MEMPROF_TAG, client::String*>* nameTags;
	client::TMap <uintptr_t, AllocationData*>* allocatedMemory;
	size_t totalMemoryAllocated;
	static TimestampManager timestampManager;
};

class [[cheerp::jsexport]] [[cheerp::genericjs]] CheerpMemProf
{
private:
public:
	CheerpMemProf()
	{
	}
	client::TArray<client::Object>* liveAllocations()
	{
		return CheerpAllocationsTracker::liveAllocations();
	}
	client::Object* liveAllocationsTree(bool isTopDown)
	{
		return CheerpAllocationsTracker::liveAllocationsTree(isTopDown);
	}
	int32_t totalLiveMemory()
	{
		return CheerpAllocationsTracker::totalLiveMemory();
	}
};

[[cheerp::wasm]] CHEERP_MEMPROF_TAG cheerpMemProfAnnotate(const char *tag_name)
{
	return CheerpAllocationsTracker::createLabel(tag_name);
}

[[cheerp::wasm]] void cheerpMemProfClose(CHEERP_MEMPROF_TAG tag)
{
	CheerpAllocationsTracker::statusTracking(tag);
	CheerpAllocationsTracker::closeTracking(tag);
}

[[cheerp::wasm]] void cheerpMemProfLive(CHEERP_MEMPROF_TAG tag)
{
	CheerpAllocationsTracker::statusTracking(tag);
}

[[cheerp::wasm]] size_t cheerpMemProfUsed(CHEERP_MEMPROF_TAG tag)
{
	return CheerpAllocationsTracker::liveMemory(tag);
}

[[cheerp::wasm]] size_t cheerpMemProfTotalUsed()
{
	return CheerpAllocationsTracker::totalLiveMemory();
}

[[cheerp::wasm]] void* _malloc_r(void* reent, size_t size)
{
	void* ret = mALLOc(reent, size);
	CheerpAllocationsTracker::registerAlloc(reinterpret_cast<uintptr_t>(ret), size);
	return ret;
}

[[cheerp::wasm]] void* _realloc_r(void* reent, void* ptr, size_t size)
{
	CheerpAllocationsTracker::registerFree(reinterpret_cast<uintptr_t>(ptr));
	void* ret = rEALLOc(reent, ptr, size);
	CheerpAllocationsTracker::registerAlloc(reinterpret_cast<uintptr_t>(ret), size);
	return ret;
}

[[cheerp::wasm]] void* _calloc_r(void* reent, size_t nmemb, size_t size)
{
	void* ret = cALLOc(reent, nmemb, size);
	CheerpAllocationsTracker::registerAlloc(reinterpret_cast<uintptr_t>(ret), nmemb*size);
	return ret;
}

[[cheerp::wasm]] void _free_r(void* reent, void* ptr)
{
	fREe(reent, ptr);
	CheerpAllocationsTracker::registerFree(reinterpret_cast<uintptr_t>(ptr));
}
