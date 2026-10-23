#include "error.hpp"

namespace tinylc {
	[[noreturn]]
	void luaError(const char* msg) {
		std::cout << "Lua: " << msg << "\n";
		std::cout << "code traceback:\n";
		std::cout << "\t[C] : ?\n";
		std::exit(EXIT_FAILURE);
	}

	[[gnu::cold, noreturn]]
	void arithmeticError(void) {
		luaError("attempt to perform arithmetic on incompatible types");
  }
} // namespace tinylc