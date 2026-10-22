#include "operator.hpp"

namespace tinylc {
	// Binary
	Value _add(const Value& a, const Value& b) {
		double av = assume_number(a);
		double bv = assume_number(b);
		return Value(av + bv);
	}

	Value _sub(const Value& a, const Value& b) {
		double av = assume_number(a);
		double bv = assume_number(b);
		return Value(av - bv);
	}

	Value _mul(const Value& a, const Value& b) {
		double av = assume_number(a);
		double bv = assume_number(b);
		return Value(av * bv);
	}

	Value _div(const Value& a, const Value& b) {
		double av = assume_number(a);
		double bv = assume_number(b);
		return Value(av / bv);
	}


	// Binary Bitwise
	Value _eq(const Value& a, const Value& b) {
		if (a.type == b.type) {
			switch (a.type) {
			case Type::NIL: {
				return Value(true);
			};
			case Type::INT: {
				return Value(a.i == b.i);
			};
			case Type::NUM: {
				return Value(a.n == b.n);
			};
			case Type::BOOL: {
				return Value(a.b == b.b);
			};
			case Type::OBJ: {
				if (a.o->type == ObjType::STRING && b.o->type == ObjType::STRING) {
					return Value(a.o == b.o);
				}
			}
			}
		}

		// INT vs NUM
		if ((a.type == Type::INT && b.type == Type::NUM) ||
			(a.type == Type::NUM && b.type == Type::INT)) {
			double av = (a.type == Type::INT) ? (double)a.i : a.n;
			double bv = (b.type == Type::INT) ? (double)b.i : b.n;
			return Value(av == bv);
		}

		return Value(false);
	}

	
	Value _lt(const Value& a, const Value& b) {
		// number < number
		if ((a.type == Type::INT || a.type == Type::NUM) &&
			(b.type == Type::INT || b.type == Type::NUM)) {
			double av = (a.type == Type::INT) ? (double)a.i : a.n;
			double bv = (b.type == Type::INT) ? (double)b.i : b.n;
			return Value(av < bv);
		}

		// string < string
		if (a.type == Type::OBJ && b.type == Type::OBJ && a.o->type == ObjType::STRING &&
			b.o->type == ObjType::STRING) {
			auto* sa = static_cast<ObjString*>(a.o);
			auto* sb = static_cast<ObjString*>(b.o);
			return Value(compareStrings(sa, sb) < 0);
		}

		luaError("attempt to compare wrong datatypes");
	}

	
	Value _gt(const Value& a, const Value& b) {
		return _lt(b, a);
	}

	
	Value _le(const Value& a, const Value& b) {
		Value lt = _lt(a, b);
		if (lt.type == Type::BOOL && lt.b) {
			return Value(true);
		}

		Value eq = _eq(a, b);
		return eq;
	}

	
	Value _ge(const Value& a, const Value& b) {
		Value lt = _lt(b, a);
		if (lt.type == Type::BOOL && lt.b) {
			return Value(true);
		}

		Value eq = _eq(a, b);
		return eq;
	}
} // namespace tinylc