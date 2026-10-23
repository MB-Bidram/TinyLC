#include "include/lua.hpp"

int main(void) {
  lua::init();

  lua::local a = 5, b = 6;

  if (a == b) {
    lua::print("A == B!");
  } else {
    lua::print("A != B");
  }

  lua::print(a, b);

  return 0;
}