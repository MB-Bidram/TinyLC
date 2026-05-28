# TinyLC 0.1-preview
**TinyLC** is an experimental Lua-like DSL implemented in C++, designed for minimal overhead and optimization-friendly execution. 
It aims to combine a simple Lua-like language with direct C++ integration, without relying on FFI.

## What the DSL aims to be
  - Simple and minimal
  - As close to Lua as possible
  - Nearly as flexible as Lua
  - Avoid C++ RTTI complexity where possible
  - Keep runtime structures simple and low-level
  - Minimal overhead
  - Near-native performance


## Performance-oriented design
- No Virtual Machine loop
- A simpler runtime design intended to remain optimization-friendly
- No traditional stack-based Virtual Machine

## Trade-offs Compared to Lua
### Limitations
  - No debug library support, due to the lack of a traditional VM and VM stack
  - Less flexible syntax ( C++ limits )
  - No support for external libraries built around the Lua C API
### Advantages
  - Native coroutine library ( Standard C++ Coroutines )
  - Support for mixing C++ with the Lua-like DSL
  - Lightweight

## Current state
  - currently a solo-developed project and remains under active development
  - No function support
  - No library support
  - No standard library
  - Some behavior is still unstable
  - Compiler portability is currently limited ( the project currently builds with GCC and Clang )


## Design Overview
  ### Value Representation
  The design uses a **Tagged Union** for its Values
  because it is more CPU-friendly and avoids C++ heavy Polymorphism
  
  ### String Interning
  Strings are **Interned**, meaning each unique string is stored only once in memory.
  The runtime computes the string hash first, then performs a lookup in a global hash map:
  - If found, return a pointer to the existing string
  - If not, allocate a new memory
  Because interned strings can be compared by pointer, equality checks avoid repeated full string comparisons.
  This also improves the efficiency of string-keyed table lookups.

  ### Garbage Collection
  The garbage collector currently uses a **mark-and-sweep** design with an **epoch-based marking optimization**.
