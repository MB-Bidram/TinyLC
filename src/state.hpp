#pragma once

#include <cstdlib>
#include <cstring>

#include "common.hpp"
#include "table.hpp"

namespace tinylc {
	struct State {
		InternNode** buckets;
		size_t capacity;
		size_t count;

		Obj* allObjects;

		ObjTable* registry;
		uint32_t freeReg;

		size_t bytesAllocated;
		size_t nextGC;
		bool gcEnabled;
		uint32_t currentEpoch;
		bool gcRunning;

		NumStrCache* numStrCache;
		uint32_t numStrCap;
		uint32_t numStrCount;

		Value** roots;
		uint32_t rootTop;
		uint32_t rootCap;

		State()
			: capacity(64)
			, count(0)
			, allObjects(nullptr)
			, bytesAllocated(0)
			, nextGC(1024 * 1024)
			, gcEnabled(true)
			, currentEpoch(1)
			, gcRunning(false)
			, freeReg(UINT32_MAX) {
			buckets = (InternNode**)xmalloc(sizeof(InternNode*) * capacity);
			std::memset(buckets, 0, sizeof(InternNode*) * capacity);
			numStrCap = 64;
			numStrCount = 0;
			numStrCache = (NumStrCache*)xmalloc(sizeof(NumStrCache) * numStrCap);
			rootTop = 0;
			rootCap = 64;
			roots = (Value**)xmalloc(sizeof(Value*) * rootCap);
		}
	};

	extern thread_local State* G;

	void init();
	State& runtime();
} // namespace tinylc