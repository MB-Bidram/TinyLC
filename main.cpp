#include "lua.hpp"

using namespace lua;

int main(void) {
    init();

    local a = 12;
    print(a);

    return 0;
}