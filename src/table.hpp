#pragma once

#include "gc.hpp"
#include "hash.hpp"

namespace tinylc {
	void markTable(State& S, Obj* obj);
	void freeTable(State& S, Obj* obj);
	ObjTable* newTable(uint32_t cap = 8);
	void tableEnsure(ObjTable* a, uint32_t index);
	Value tableGet(ObjTable* a, uint32_t i);
	void tableSet(ObjTable* a, uint32_t index, const Value& v);
	void promoteIntToNum(ObjTable* a);
	void promoteToMixed(ObjTable* a);
} // namespace tinylc