#pragma once

#include <cstdlib>
#include <iostream>

#include "state.hpp"
#include "table.hpp"

namespace tinylc {
	void markObject(State&, Obj*);
	void collectGarbage(State& S);

	void growRoots(State& S);
	void maybeCollect(State& S);
	void markInternTable(State& S);

	// Memory
	void* allocate(State& S, size_t size);
	void deallocate(State& S, void* p, size_t size);
	void removeIntern(State& S, ObjString* victim);
	void* xmalloc(size_t size);

	// Debug
	void debugListObjects(State& S);

	// Garbage Collector
	[[gnu::cold]]
	void markObject(State& S, Obj* obj);
	void freeString(State& S, Obj* obj);
	void sweep(State& S);
	[[gnu::cold]]
	void collectGarbage(State& S);

	// Registery
	uint32_t registryAdd(State& S, const Value& v);
	void registryRemove(State& S, uint32_t idx);
} // namespace tinylc