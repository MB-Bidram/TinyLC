#pragma once

#include <cstddef>
#include <cstdint>

namespace tinylc {
	struct Obj;
	struct State;

	using FreeFn = void (*)(State&, Obj*);
	void* xmalloc(size_t size);


	enum class Type : uint8_t {
		NIL,
		INT,
		NUM,
		BOOL,
		OBJ
	};

	enum class ObjType : uint8_t {
		TABLE,
		// FUNCTION,
		// THREAD,
		STRING,
	};


	struct Obj {
		ObjType type;
		uint32_t markEpoch;
		Obj* next;
		FreeFn free;
	};


	struct Value {
		union {
			double n;
			int64_t i;
			Obj* o;
			bool b;
		};
		Type type;

		constexpr Value()
			: type(Type::NIL)
			, o(nullptr) {}
		constexpr Value(int64_t v)
			: type(Type::INT)
			, i(v) {}
		constexpr Value(double v)
			: type(Type::NUM)
			, n(v) {}
		constexpr Value(bool v)
			: type(Type::BOOL)
			, b(v) {}
		constexpr Value(Obj* v)
			: type(Type::OBJ)
			, o(v) {}
	};


	struct ObjString : Obj {
		size_t length;
		uint32_t hash;
		char* chars;
	};


	struct ObjTable : Obj {
		uint32_t size;
		uint32_t capacity;

		enum class ArrayKind : uint8_t {
			INT,
			NUM,
			MIXED
		} kind;

		union {
			int64_t* ints;
			double* nums;
			Value* values;
		};

		void* hash; // opaque cold pointer
	};

	struct HashEntry {
		Value key;
		Value value;
	};

	struct ObjTableHash {
		HashEntry* data;
		uint32_t cap;
		uint32_t count;
	};

	enum class GCOp {
		STOP,
		RESTART,
		COLLECT,
		COUNT,
		STEP
	};

	struct InternNode {
		ObjString* str;
		InternNode* next;
	};

	struct NumStrCache {
		double num;
		ObjString* str;
	};
} // namespace tinylc