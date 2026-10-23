#pragma once

#include <cstring>

#include "common.hpp"
#include "error.hpp"
#include "gc.hpp"
#include "state.hpp"

namespace tinylc {
	bool keyEqual(const Value& a, const Value& b);
	uint32_t hashString(const char* s, size_t len);
	Value tableGetRaw(ObjTable* a, uint32_t i);
	uint32_t hashValue(const Value& k);
	void ensureHash(ObjTable* t);
	ObjTableHash* tableHash(ObjTable* t);
	HashEntry* hashFind(ObjTable* t, const Value& key);
} // namespace tinylc