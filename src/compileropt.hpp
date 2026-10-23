#pragma once

#include "common.hpp"
#include <cstring>

namespace tinylc {
	double numberSlow(Value v);

	inline double assume_number(const Value& v) {
		if (v.type == Type::INT)
			return double(v.i);
		if (v.type == Type::NUM)
			return v.n;
		return numberSlow(v);
	}

	inline int64_t integerFromBits(uint64_t bits) {
		int64_t result;
		std::memcpy(&result, &bits, sizeof(result));
		return result;
	}
} // namespace tinylc
