#pragma once

#include "common.hpp"
#include "error.hpp"
#include "compileropt.hpp"
#include "string.hpp"


namespace tinylc {
	// Binary
	Value _add(const Value& a, const Value& b);
	Value _sub(const Value& a, const Value& b);
	Value _mul(const Value& a, const Value& b);
	Value _div(const Value& a, const Value& b);

	// Binary Bitwise
	Value _eq(const Value& a, const Value& b);
	Value _lt(const Value& a, const Value& b);
	Value _gt(const Value& a, const Value& b);
	Value _le(const Value& a, const Value& b);
	Value _ge(const Value& a, const Value& b);
} // namespace tinylc