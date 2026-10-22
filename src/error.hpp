#pragma once

#include <iostream>

namespace tinylc {
    [[noreturn]]
    void luaError(const char* msg);

    [[gnu::cold, noreturn]]
    void arithmeticError(void);
} // namespace tinylc