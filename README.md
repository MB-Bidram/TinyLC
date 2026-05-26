**Lua DSL in C++** is a Lua like DSL written in C++ aiming to have Minimal overhead with near Native speed,
Making Bindings more Easy without FFI and Direct use of both C++ and a Simple as possible Lua like Language.

## How could it reach Native speed?
- No Virtual Machine loop
- No complex design ( To keep everything optimization friendly )
- No stack

## What are the Cons and Pros from the Vanilla Lua?
- ## Cons
  - No existanse of the Debug library ( There is no Stack or VM )
  - Less flexible syntax ( C++ limits )
  - No external Lua C API binded library support
- ## Pros
  - Native coroutine library ( Standard C++ Coroutines )
  - Support for mixture of C++ and Lua DSL
  - Lightweight

## Current state
  - Currently the project is solo and slow
  - No function support
  - No library support
  - No standard library
  - Buggy
  - Not portable accross C++ Compilers ( GNU G++ and Clang can Compile the source code )


## More info about the Design
  The design uses a **Tagged Union** for its Values
  Because its more CPU friendly and avoids C++ heavy Polymorphism
  
  Strings are **Interned**, So they are stored only once in the Memory
  It works by calculating the Hash of the string first,
  Then does a lookup in the Global Hash Map.
  - If found, return a pointer to the existing string
  - If not, allocate a new memory
  as memcmp is slow and expensive, the design uses a Pointer comarison to check if they are equal or not
  because the Hash is pre-calculated and stored, it makes table lookups with string extremely fast.

  The GC uses a Mark-And-Sweep design with Epoch-Based marking Optimization.


## What the DSL aims to be
  - Simple and minimal
  - Near Lua as possible
  - Flexible
  - Avoid C++ RTTS / RAII complexity and overhead
  - Minimal overhead
  - Near Native Speed
