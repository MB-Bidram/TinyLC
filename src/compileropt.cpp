#include "compileropt.hpp"

namespace tinylc {
	double assume_number(const Value& v) {
		TINYLC_ASSUME(v.type == Type::NUM || v.type == Type::INT);

		return (v.type == Type::INT) ? (double)v.i : v.n;
	}
} // namespace tinylc