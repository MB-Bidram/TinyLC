#pragma once 

#include <iostream>

#include "common.hpp"
#include "state.hpp"
#include "string.hpp"


namespace tinylc {
	void _printValue(const Value& v);
	const char* typeName(Type t);
	double gc(State& S, GCOp op);
	Value toString(const Value& v);
	Value tonumber(const Value& v);
	Value makeTable();
} // namespace tinylc