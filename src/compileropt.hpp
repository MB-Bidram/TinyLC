#pragma once

#include "common.hpp"

#if defined(_MSC_VER)
#define TINYLC_ASSUME(cond) __assume(cond)
#elif defined(__clang__)
#define TINYLC_ASSUME(cond) __builtin_assume(cond)
#elif defined(__GNUC__)
#define TINYLC_ASSUME(cond)                                                                        \
	do {                                                                                           \
		if (!(cond))                                                                               \
			__builtin_unreachable();                                                               \
	} while (0)
#else
#define TINYLC_ASSUME(cond) ((void)0)
#endif

namespace tinylc {
	double assume_number(const Value& v);
} // namespace tinylc