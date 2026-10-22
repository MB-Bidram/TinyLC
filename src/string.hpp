#pragma once

#include "gc.hpp"
#include "common.hpp"
#include "state.hpp"


namespace tinylc {
	const char* objTypeName(ObjType t);
	void _printObject(Obj* obj);
	bool stringToNumber(const ObjString* s, double& out);
	bool toNumber(const Value& v, double& out);
	void growInternTable(State& S);
	ObjString* newString(const char* chars, size_t length);
	Value makeString(const char* s);
	int compareStrings(const ObjString* a, const ObjString* b);
} // namespace tinylc