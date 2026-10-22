# TinyLC
### **Current version: 1.0.0**
**TinyLC** is an experimental Lua-like DSL implemented in C++, designed for minimal overhead and optimization-friendly execution. 
It aims to combine a simple Lua-like language with direct C++ integration, without relying on FFI.

## Quick start
### Clone the repo
```bash
git clone https://github.com/MB-Bidram/TinyLC.git
cd TinyLC
```
### Compile (Windows only)
```bash
compile.bat
```
### Run
```bash
./build/tinylc-main.exe
```
Note:
> The project was only tested on GNU GCC 15.2.0 and im not sure about compiler protability.
> compile.bat uses gcc as the main compiler.
---

## Usage
TinyLC uses a slightly different API than Lua itself (because of C++ limits)

### Basic usage
Before using TinyLC's API, initialize its runtime with `lua::init()`.
The following example demonstrates initializing TinyLC, declaring local values, comparing them, and printing the results.
```cpp
#include "include/lua.hpp"

int main(void) {
  lua::init();

  lua::local a,b = 5, 6;

  if (a == b) {
    lua::print("A == B!");
  } else {
    lua::print("A != B");
  }

  lua::print(a, b);

  return 0;
}
```
Output
```txt
A != B
5 6
```
---

## What TinyLC aims to be
  - Simple and minimal
  - As close to Lua as possible
  - Nearly as flexible as Lua
  - Minimal overhead
  - Near-native performance

## Performance-oriented design
- No Virtual Machine loop
- A simpler runtime design intended to remain optimization-friendly
- No traditional stack-based Virtual Machine

## Trade-offs Compared to Lua
### Limitations
  - No debug library support, due to the lack of a traditional VM and VM stack
  - Less flexible syntax (C++ limits)
  - No support for external libraries built around the Lua C API (For now)
### Advantages
  - Native coroutine library (Standard C++ Coroutines, Not implemented 'yet') 
  - Support for mixing C++ with the Lua-like DSL
  - Lightweight

## Current state
  - currently a solo-developed project
  - No function support
  - No library support
  - No complete standard library
  - Some behaviors are still unstable

## License

TinyLC is licensed under the Apache License 2.0.

You are free to use, modify, and distribute this project under the terms of the license. See the [LICENSE](LICENSE) file for the full text.
