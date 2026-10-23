#include "compileropt.hpp"
#include "error.hpp"
#include "stdlib.hpp"

namespace tinylc {
	double numberSlow(Value v) {
		double result;
		if (toNumber(v, result))
			return result;
		arithmeticError();
	}
} // namespace tinylc
