#pragma once

#include "common.hpp"
#include "compileropt.hpp"

namespace tinylc {
	// Only string comparison and its error path need the object runtime.
	int compareObjects(Value a, Value b);

	inline bool isNumber(const Value& v) {
		return v.type == Type::INT || v.type == Type::NUM;
	}

	// Preserve the DSL's existing double-valued binary arithmetic.
	inline Value _add(const Value& a, const Value& b) {
		return Value(assume_number(a) + assume_number(b));
	}
	inline Value _sub(const Value& a, const Value& b) {
		return Value(assume_number(a) - assume_number(b));
	}
	inline Value _mul(const Value& a, const Value& b) {
		return Value(assume_number(a) * assume_number(b));
	}
	inline Value _div(const Value& a, const Value& b) {
		return Value(assume_number(a) / assume_number(b));
	}

	inline Value _eq(const Value& a, const Value& b) {
		if (a.type == b.type) {
			switch (a.type) {
			case Type::NIL:
				return Value(true);
			case Type::INT:
				return Value(a.i == b.i);
			case Type::NUM:
				return Value(a.n == b.n);
			case Type::BOOL:
				return Value(a.b == b.b);
			case Type::OBJ:
				return Value(a.o == b.o);
			}
		}
		if (isNumber(a) && isNumber(b))
			return Value(assume_number(a) == assume_number(b));
		return Value(false);
	}

	inline Value _lt(const Value& a, const Value& b) {
		if (a.type == Type::INT && b.type == Type::INT)
			return Value(a.i < b.i);
		if (isNumber(a) && isNumber(b))
			return Value(assume_number(a) < assume_number(b));
		return Value(compareObjects(a, b) < 0);
	}
	inline Value _gt(const Value& a, const Value& b) {
		return _lt(b, a);
	}
	inline Value _le(const Value& a, const Value& b) {
		return Value(_lt(a, b).b || _eq(a, b).b);
	}
	inline Value _ge(const Value& a, const Value& b) {
		return _le(b, a);
	}
} // namespace tinylc
