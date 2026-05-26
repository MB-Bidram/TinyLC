#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <cstring>



namespace tinylc {
	// Forward Declerations ( might be messy )
	struct Obj;
	struct State;

	using FreeFn = void(*)(State&, Obj*);
	inline void* xmalloc(size_t size);


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
			double  n;
			int64_t i;
			Obj* o;
			bool    b;
		};
		Type type;

		constexpr Value() : type(Type::NIL), o(nullptr) {}
		constexpr Value(int64_t v) : type(Type::INT), i(v) {}
		constexpr Value(double  v) : type(Type::NUM), n(v) {}
		constexpr Value(bool    v) : type(Type::BOOL), b(v) {}
		constexpr Value(Obj* v) : type(Type::OBJ), o(v) {}
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
		uint32_t   cap;
		uint32_t   count;
	};


	// Error
	[[noreturn]]
	inline void luaError(const char* msg) {
		std::cout << "Lua: " << msg << "\n";
		std::cout << "code traceback:\n";
		std::cout << "\t[C] : ?\n";
		std::exit(EXIT_FAILURE);
	}

	[[gnu::cold, noreturn]]
	void arithmeticError() {
		luaError("attempt to perform arithmetic on incompatible types");
	}


	// Compiler Optimization
#if defined(_MSC_VER)
#define TINYLC_ASSUME(cond) __assume(cond)
#elif defined(__clang__)
#define TINYLC_ASSUME(cond) __builtin_assume(cond)
#elif defined(__GNUC__)
#define TINYLC_ASSUME(cond) \
	        do { if (!(cond)) __builtin_unreachable(); } while (0)
#else
#define TINYLC_ASSUME(cond) ((void)0)
#endif


	[[gnu::always_inline]]
	inline double assume_number(const Value& v) {
		TINYLC_ASSUME(
			v.type == Type::NUM ||
			v.type == Type::INT
		);

		return (v.type == Type::INT)
			? (double)v.i
			: v.n;
	}



	// Runtime
	inline void* allocate(State& S, size_t size);
	inline void deallocate(State& S, void* p, size_t size);

	struct InternNode {
		ObjString* str;
		InternNode* next;
	};

	struct NumStrCache {
		double     num;
		ObjString* str;
	};



	inline void markTable(State& S, Obj* obj);
	inline tinylc::ObjTable* newTable(uint32_t cap);
	inline void tableSet(tinylc::ObjTable* a, uint32_t index, const tinylc::Value& v);


	struct State {
		InternNode** buckets;
		size_t capacity;
		size_t count;

		Obj* allObjects;

		ObjTable* registry;
		uint32_t freeReg;

		size_t bytesAllocated;
		size_t nextGC;
		bool gcEnabled;
		uint32_t currentEpoch;
		bool gcRunning;

		NumStrCache* numStrCache;
		uint32_t     numStrCap;
		uint32_t     numStrCount;

		tinylc::Value** roots;
		uint32_t rootTop;
		uint32_t rootCap;

		State()
			: capacity(64),
			count(0),
			allObjects(nullptr),
			bytesAllocated(0),
			nextGC(1024 * 1024), // 1MB initial threshold
			gcEnabled(true),
			currentEpoch(1),
			gcRunning(false),
			freeReg(UINT32_MAX)
		{
			buckets = (InternNode**)xmalloc(sizeof(InternNode*) * capacity);
			std::memset(buckets, 0, sizeof(InternNode*) * capacity); numStrCap = 64;
			numStrCount = 0; numStrCache = (NumStrCache*)xmalloc(sizeof(NumStrCache) * numStrCap); 
			rootTop = 0; rootCap = 64;
			roots = (Value**)xmalloc(sizeof(Value*) * rootCap);
		}
	};

	inline thread_local State* G = nullptr;


	inline void init() {
		if (!G) {
			G = (State*)xmalloc(sizeof(State));
			new (G) State();

			G->registry = newTable(8);
			G->registry->kind = ObjTable::ArrayKind::MIXED;
			G->registry->values =
				(Value*)allocate(*G, sizeof(Value) * G->registry->capacity);

			for (uint32_t i = 0; i < G->registry->capacity; ++i)
				G->registry->values[i] = Value();
		}
	}

	inline State& runtime() {
		if (!G) luaError("Lua state not initialized");
		return *G;
	}



	// GC Helper
	inline void markObject(State&, Obj*);
	inline void collectGarbage(State& S);

	inline void growRoots(State& S) {
		uint32_t newCap = S.rootCap * 2;
		S.roots = (Value**)std::realloc(S.roots, sizeof(Value*) * newCap);
		S.rootCap = newCap;
	}

	inline void maybeCollect(State& S) {
		if (!S.gcEnabled) return;

		if (S.bytesAllocated >= S.nextGC) {
			collectGarbage(S);

			// grow threshold
			S.nextGC = S.bytesAllocated * 2 + 1024;
		}
	}


	inline void markInternTable(State& S) {
		for (size_t i = 0; i < S.capacity; ++i) {
			for (InternNode* node = S.buckets[i]; node; node = node->next)
				markObject(S, (Obj*)node->str);
		}
	}



	// Memory
	inline void* allocate(State& S, size_t size) {
		S.bytesAllocated += size;
		if (!S.gcRunning && S.gcEnabled && S.bytesAllocated >= S.nextGC)
			maybeCollect(S);
		return xmalloc(size);
	}

	inline void deallocate(State& S, void* p, size_t size) {
		if (S.bytesAllocated >= size) {
			S.bytesAllocated -= size;
		}
		else {
			S.bytesAllocated = 0;  // Prevent underflow
		}
		std::free(p);
	}


	inline void removeIntern(State& S, ObjString* victim) {
		size_t index = victim->hash % S.capacity;
		InternNode** p = &S.buckets[index];

		while (*p) {
			if ((*p)->str == victim) {
				InternNode* dead = *p;
				*p = dead->next;
				deallocate(S, dead, sizeof(InternNode));
				return;
			}
			p = &(*p)->next;
		}
	}



	inline void* xmalloc(size_t size) {
		void* p = malloc(size);
		if (!p) luaError("memory allocation failed");
		return p;
	}



	// Debug
	inline void debugListObjects(State& S) {
		size_t count = 0;
		for (Obj* o = S.allObjects; o; o = o->next) {
			count++;
			std::cout << "[" << count << "] "
				<< "obj=" << o
				<< " type=" << (int)o->type
				<< " marked=" << (int)o->markEpoch
				<< "\n";
		}
		if (count == 0)
			std::cout << "(no objects)\n";
	}



	// Helper
	inline const char* objTypeName(ObjType t) {
		switch (t) {
		case ObjType::STRING:  return "string";
			// case ObjType::TABLE:   return "table";
			// case ObjType::FUNCTION:return "function";
			// case ObjType::THREAD:  return "thread";
		}
		return "object";
	}

	inline void _printObject(Obj* obj) {
		switch (obj->type) {

		case ObjType::STRING: {
			auto* s = static_cast<ObjString*>(obj);
			std::cout << s->chars;
			break;
		}

		default: {
			std::cout
				<< objTypeName(obj->type)
				<< ": "
				<< "0x" << std::hex << (uintptr_t)obj << std::dec;
			break;
		}
		}
	}



	inline bool stringToNumber(const ObjString* s, double& out) {
		char* end;
		out = std::strtod(s->chars, &end);

		// conversion failed if no chars consumed
		if (end == s->chars)
			return false;

		// skip trailing whitespace
		while (*end == ' ' || *end == '\t' || *end == '\n')
			end++;

		// must consume whole string
		return *end == '\0';
	}


	inline bool toNumber(const Value& v, double& out) {
		switch (v.type) {
		case Type::INT:
			out = (double)v.i;
			return true;

		case Type::NUM:
			out = v.n;
			return true;

		case Type::OBJ:
			if (v.o->type == ObjType::STRING) {
				return stringToNumber(
					static_cast<ObjString*>(v.o),
					out
				);
			}
			return false;

		default:
			return false;
		}
	}

	inline bool bothInts(const Value& a, const Value& b) {
		return a.type == Type::INT && b.type == Type::INT;
	}

	inline uint32_t hashString(const char* s, size_t len) {
		uint32_t h = 2166136261u;
		for (size_t i = 0; i < len; i++) {
			h ^= (uint8_t)s[i];
			h *= 16777619u;
		}
		return h;
	}

	inline Value tableGetRaw(ObjTable* a, uint32_t i) {
		if (i >= a->size) return Value();  // Safety check

		auto kind = a->kind;
		switch (kind) {
		case ObjTable::ArrayKind::INT:   return Value(a->ints[i]);
		case ObjTable::ArrayKind::NUM:   return Value(a->nums[i]);
		case ObjTable::ArrayKind::MIXED: return a->values[i];
		}
		return {};
	}

	inline uint32_t hashValue(const Value& k) {
		switch (k.type) {
		case Type::INT:  return (uint32_t)k.i * 2654435761u;
		case Type::NUM: {
			uint64_t u;
			std::memcpy(&u, &k.n, sizeof(u));
			return (uint32_t)(u ^ (u >> 32));
		}
		case Type::BOOL: return k.b ? 0xABCDEF01u : 0x12345678u;
		case Type::OBJ:
			if (k.o->type == ObjType::STRING)
				return static_cast<ObjString*>(k.o)->hash;
			return (uint32_t)(uintptr_t)k.o;
		default:
			return 0;
		}
	}

	inline bool keyEqual(const Value& a, const Value& b) {
		if (a.type != b.type) {
			// INT vs NUM numeric equality
			if ((a.type == Type::INT && b.type == Type::NUM) ||
				(a.type == Type::NUM && b.type == Type::INT)) {
				double av = (a.type == Type::INT) ? (double)a.i : a.n;
				double bv = (b.type == Type::INT) ? (double)b.i : b.n;
				return av == bv;
			}
			return false;
		}

		switch (a.type) {
		case Type::INT:  return a.i == b.i;
		case Type::NUM:  return a.n == b.n;
		case Type::BOOL: return a.b == b.b;
		case Type::OBJ:  return a.o == b.o;
		case Type::NIL:  return true;
		}
		return false;
	}

	inline void ensureHash(ObjTable* t) {
		if (t->hash) return;

		State& S = runtime();

		ObjTableHash* h = (ObjTableHash*)allocate(S, sizeof(ObjTableHash));
		h->cap = 8;
		h->count = 0;
		h->data = (HashEntry*)allocate(S, sizeof(HashEntry) * h->cap);

		for (uint32_t i = 0; i < h->cap; ++i) {
			h->data[i].key = Value();
			h->data[i].value = Value();
		}

		t->hash = h;
	}

	inline ObjTableHash* tableHash(ObjTable* t) {
		return (ObjTableHash*)t->hash;
	}


	inline HashEntry* hashFind(ObjTable* t, const Value& key) {
		if (!t->hash)
			luaError("internal error: hashFind on table without hash");
		ObjTableHash* h = tableHash(t);
		uint32_t mask = h->cap - 1;

		uint32_t i = hashValue(key) & mask;
		for (;;) {
			HashEntry* e = &h->data[i];
			if (e->key.type == Type::NIL || keyEqual(e->key, key))
				return e;
			i = (i + 1) & mask;
		}
	}



	// Garbage Collector
	[[gnu::cold]]
	inline void markObject(State& S, Obj* obj) {
		if (!obj) return;

		// sanity: object from future?
		if (obj->markEpoch > S.currentEpoch) {
			std::abort();
		}

		if (obj->markEpoch == S.currentEpoch)
			return;

		obj->markEpoch = S.currentEpoch;

		if (obj->type == ObjType::TABLE)
			markTable(S, obj);
	}

	inline void freeString(State& S, Obj* obj) {
		auto* s = (ObjString*)obj;
		removeIntern(S, s);
		deallocate(S, s->chars, s->length + 1);
		deallocate(S, s, sizeof(ObjString));
	}


	inline void sweep(State& S) {
		Obj** p = &S.allObjects;
		while (*p) {
			Obj* obj = *p;
			if (obj->markEpoch != S.currentEpoch) {
				*p = obj->next;
				obj->free(S, obj);
			}
			else {
				p = &obj->next;
			}
		}
	}

	[[gnu::cold]]
	inline void collectGarbage(State& S) {
		//debugListObjects(S);

		S.gcRunning = true;
		S.currentEpoch++;

		// ---- Mark Phase (roots)
		markObject(S, (Obj*)S.registry);
		markInternTable(S);

		// MARK LOCALS (root stack)
		for (uint32_t i = 0; i < S.rootTop; ++i) {
			tinylc::Value* v = S.roots[i];
			if (v && v->type == tinylc::Type::OBJ)
				markObject(S, v->o);
		}


		// ---- Sweep Phase
		sweep(S);

		S.nextGC = S.bytesAllocated * 2 + 1024;
		S.gcRunning = false;
	}



	// Table
	inline void markTable(State& S, Obj* obj) {
		auto* t = (ObjTable*)obj;

		// array
		if (t->kind == ObjTable::ArrayKind::MIXED) {
			for (uint32_t i = 0; i < t->size; ++i)
				if (t->values[i].type == Type::OBJ)
					markObject(S, t->values[i].o);
		}

		// hash
		if (t->hash) {
			ObjTableHash* h = tableHash(t);
			for (uint32_t i = 0; i < h->cap; ++i) {
				if (h->data[i].key.type == Type::OBJ)
					markObject(S, h->data[i].key.o);
				if (h->data[i].value.type == Type::OBJ)
					markObject(S, h->data[i].value.o);
			}
		}
	}

	inline void freeTable(State& S, Obj* obj) {
		auto* t = (ObjTable*)obj;

		// Save everything we need BEFORE accessing any union members
		auto kind = t->kind;
		auto capacity = t->capacity;
		auto hash = t->hash;

		// Get the correct array pointer based on current kind
		void* arrayPtr = nullptr;
		size_t elemSize = 0;

		switch (kind) {
		case ObjTable::ArrayKind::INT:
			arrayPtr = t->ints;
			elemSize = sizeof(int64_t);
			break;
		case ObjTable::ArrayKind::NUM:
			arrayPtr = t->nums;
			elemSize = sizeof(double);
			break;
		case ObjTable::ArrayKind::MIXED:
			arrayPtr = t->values;
			elemSize = sizeof(Value);
			break;
		}

		// Deallocate hash part first (if exists)
		if (hash) {
			ObjTableHash* h = (ObjTableHash*)hash;
			deallocate(S, h->data, sizeof(HashEntry) * h->cap);
			deallocate(S, h, sizeof(ObjTableHash));
		}

		// Deallocate array part
		if (arrayPtr) {
			deallocate(S, arrayPtr, elemSize * capacity);
		}

		// Finally deallocate the table object itself
		deallocate(S, t, sizeof(ObjTable));
	}

	inline ObjTable* newTable(uint32_t cap = 8) {
		State& S = runtime();

		auto* a = (ObjTable*)allocate(S, sizeof(ObjTable));

		a->type = ObjType::TABLE;
		a->markEpoch = S.currentEpoch;
		a->free = freeTable;
		a->hash = nullptr;

		a->size = 0;
		a->capacity = cap;
		a->kind = ObjTable::ArrayKind::INT;
		a->ints = (int64_t*)allocate(S, sizeof(int64_t) * cap);
		std::memset(a->ints, 0, sizeof(int64_t) * cap);

		a->next = S.allObjects;
		S.allObjects = a;

		return a;
	}

	inline void tableEnsure(ObjTable* a, uint32_t index) {
		if (index < a->capacity)
			return;

		uint32_t newCap = a->capacity;
		while (newCap <= index)
			newCap *= 2;

		State& S = runtime();

		switch (a->kind) {
		case ObjTable::ArrayKind::INT: {
			int64_t* p = (int64_t*)allocate(S, sizeof(int64_t) * newCap);
			std::memcpy(p, a->ints, sizeof(int64_t) * a->capacity);
			std::memset(p + a->capacity, 0, sizeof(int64_t) * (newCap - a->capacity));
			deallocate(S, a->ints, sizeof(int64_t) * a->capacity);
			a->ints = p;
			break;
		}

		case ObjTable::ArrayKind::NUM: {
			double* p = (double*)allocate(S, sizeof(double) * newCap);
			std::memcpy(p, a->nums, sizeof(double) * a->capacity);
			std::memset(p + a->capacity, 0, sizeof(double) * (newCap - a->capacity));
			deallocate(S, a->nums, sizeof(double) * a->capacity);
			a->nums = p;
			break;
		}

		case ObjTable::ArrayKind::MIXED: {
			Value* p = (Value*)allocate(S, sizeof(Value) * newCap);
			std::memcpy(p, a->values, sizeof(Value) * a->capacity);
			for (uint32_t i = a->capacity; i < newCap; ++i)
				p[i] = Value();
			deallocate(S, a->values, sizeof(Value) * a->capacity);
			a->values = p;
			break;
		}
		}

		a->capacity = newCap;
	}



	inline void promoteIntToNum(ObjTable* a) {
		State& S = runtime();

		double* p = (double*)allocate(S, sizeof(double) * a->capacity);
		for (uint32_t i = 0; i < a->size; ++i)
			p[i] = (double)a->ints[i];

		deallocate(S, a->ints, sizeof(int64_t) * a->capacity);
		a->nums = p;
		a->kind = ObjTable::ArrayKind::NUM;
	}

	inline void promoteToMixed(ObjTable* a) {
		State& S = runtime();

		Value* p = (Value*)allocate(S, sizeof(Value) * a->capacity);

		if (a->kind == ObjTable::ArrayKind::INT) {
			for (uint32_t i = 0; i < a->size; ++i)
				p[i] = Value(a->ints[i]);
			deallocate(S, a->ints, sizeof(int64_t) * a->capacity);
		}
		else {
			for (uint32_t i = 0; i < a->size; ++i)
				p[i] = Value(a->nums[i]);
			deallocate(S, a->nums, sizeof(double) * a->capacity);
		}

		for (uint32_t i = a->size; i < a->capacity; ++i)
			p[i] = Value();

		a->values = p;
		a->kind = ObjTable::ArrayKind::MIXED;
	}




	// String
	inline void growInternTable(State& S) {
		size_t newCap = S.capacity * 2;

		InternNode** newBuckets =
			(InternNode**)xmalloc(sizeof(InternNode*) * newCap);

		std::memset(newBuckets, 0, sizeof(InternNode*) * newCap);

		for (size_t i = 0; i < S.capacity; ++i) {
			InternNode* node = S.buckets[i];
			while (node) {
				InternNode* next = node->next;

				size_t index = node->str->hash % newCap;
				node->next = newBuckets[index];
				newBuckets[index] = node;

				node = next;
			}
		}

		std::free(S.buckets);
		S.buckets = newBuckets;
		S.capacity = newCap;
	}


	inline ObjString* newString(const char* chars, size_t length) {
		State& S = runtime();

		uint32_t hash = hashString(chars, length);
		size_t index = hash % S.capacity;

		for (InternNode* it = S.buckets[index]; it; it = it->next) {
			ObjString* s = it->str;
			if (s->hash == hash &&
				s->length == length &&
				std::memcmp(s->chars, chars, length) == 0)
				return s;
		}

		auto* s = (ObjString*)allocate(S, sizeof(ObjString));
		s->type = ObjType::STRING;
		s->markEpoch = S.currentEpoch;
		s->free = freeString;
		s->length = length;
		s->hash = hash;
		s->chars = (char*)allocate(S, length + 1);

		s->next = S.allObjects;
		S.allObjects = s;

		std::memcpy(s->chars, chars, length);
		s->chars[length] = '\0';

		auto* node = (InternNode*)allocate(S, sizeof(InternNode));
		node->str = s;
		node->next = S.buckets[index];
		S.buckets[index] = node;

		S.count++;
		if (S.count > S.capacity * 0.75)
			growInternTable(S);

		return s;
	}

	inline Value makeString(const char* s) {
		return Value(newString(s, std::strlen(s)));
	}

	inline int compareStrings(const ObjString* a, const ObjString* b) {
		size_t minLen = a->length < b->length ? a->length : b->length;
		int cmp = std::memcmp(a->chars, b->chars, minLen);
		if (cmp != 0) return cmp;
		if (a->length == b->length) return 0;
		return (a->length < b->length) ? -1 : 1;
	}



	// Standard Lib
	inline void _printValue(const Value& v) {
		switch (v.type) {

		case Type::INT:
			std::cout << v.i;
			break;

		case Type::NUM:
			std::cout << v.n;
			break;

		case Type::BOOL:
			std::cout << (v.b ? "true" : "false");
			break;

		case Type::NIL:
			std::cout << "nil";
			break;

		case Type::OBJ:
			_printObject(v.o);
			break;
		}
	}

	inline const char* typeName(Type t) {
		switch (t) {
		case Type::NIL:  return "nil";
		case Type::INT:  return "number";
		case Type::NUM:  return "number";
		case Type::BOOL: return "boolean";
		case Type::OBJ:  return "object";
		}
		return "unknown";
	}

	enum class GCOp {
		STOP,
		RESTART,
		COLLECT,
		COUNT,
		STEP
	};

	inline double gc(State& S, GCOp op) {
		switch (op) {
		case GCOp::STOP:
			S.gcEnabled = false;
			return 0;

		case GCOp::RESTART:
			S.gcEnabled = true;
			return 0;

		case GCOp::COLLECT:
			if (S.gcEnabled)
				collectGarbage(S);
			return 0;

		case GCOp::COUNT:
			return (double)S.bytesAllocated / 1024.0;

		case GCOp::STEP:
			if (S.gcEnabled)
				collectGarbage(S);
			return 0;
		}

		return 0;
	}

	inline Value toString(const Value& v) {
		State& S = runtime();

		switch (v.type) {

		case Type::NIL:
			return makeString("nil");

		case Type::BOOL:
			return makeString(v.b ? "true" : "false");

		case Type::INT:
		case Type::NUM: {
			double n = (v.type == Type::INT)
				? (double)v.i
				: v.n;

			// Cached numeric strings
			for (uint32_t i = 0; i < S.numStrCount; ++i) {
				if (S.numStrCache[i].num == n)
					return Value((Obj*)S.numStrCache[i].str);
			}

			char buf[64];

			if (v.type == Type::INT)
				std::snprintf(buf, sizeof(buf),
					"%lld", (long long)v.i);
			else
				std::snprintf(buf, sizeof(buf),
					"%.14g", n);

			ObjString* s = newString(buf, std::strlen(buf));

			if (S.numStrCount < S.numStrCap) {
				S.numStrCache[S.numStrCount++] = { n, s };
			}
			else if (S.numStrCap > 0) {
				uint32_t slot =
					((uint32_t)n) & (S.numStrCap - 1);

				S.numStrCache[slot] = { n, s };
			}

			return Value((Obj*)s);
		}

		case Type::OBJ: {

			// Lua strings return themselves
			if (v.o->type == ObjType::STRING)
				return v;

			char buf[128];

			switch (v.o->type) {

			case ObjType::TABLE:
				std::snprintf(
					buf,
					sizeof(buf),
					"table: %p",
					(void*)v.o
				);
				return makeString(buf);
			default:
				std::snprintf(
					buf,
					sizeof(buf),
					"object: %p",
					(void*)v.o
				);
				return makeString(buf);
			}
		}
		}

		return makeString("unknown");
	}

	inline Value tonumber(const Value& v) {
		double out;
		if (toNumber(v, out)) {
			if ((int64_t)out == out)
				return Value((int64_t)out);
			return Value(out);
		}
		return Value(); // nil
	}

	inline Value tableGet(ObjTable* a, uint32_t i) {
		if (i >= a->size)
			return Value(); // NIL
		return tableGetRaw(a, i);
	}

	inline void tableSet(ObjTable* a, uint32_t index, const Value& v) {
		tableEnsure(a, index);

		switch (a->kind) {
		case ObjTable::ArrayKind::INT:
			if (v.type == Type::INT) {
				a->ints[index] = v.i;
				break;
			}
			if (v.type == Type::NUM) {
				promoteIntToNum(a);
				a->nums[index] = v.n;
				break;
			}
			promoteToMixed(a);
			a->values[index] = v;
			break;

		case ObjTable::ArrayKind::NUM:
			if (v.type == Type::INT || v.type == Type::NUM) {
				a->nums[index] = (v.type == Type::INT) ? (double)v.i : v.n;
				break;
			}
			promoteToMixed(a);
			a->values[index] = v;
			break;

		case ObjTable::ArrayKind::MIXED:
			a->values[index] = v;
			break;
		}

		if (index >= a->size)
			a->size = index + 1;
	}

	inline Value makeTable() {
		return Value((Obj*)newTable());
	}



	// Registery
	inline uint32_t registryAdd(State& S, const Value& v) {
		ObjTable* r = S.registry;

		// reuse free slot if available
		if (S.freeReg != UINT32_MAX) {
			uint32_t idx = S.freeReg;
			Value next = tableGet(r, idx);
			S.freeReg = (next.type == Type::INT)
				? (uint32_t)next.i
				: UINT32_MAX;

			tableSet(r, idx, v);
			return idx;
		}

		uint32_t idx = r->size;
		tableSet(r, idx, v);
		return idx;
	}

	inline void registryRemove(State& S, uint32_t idx) {
		ObjTable* r = S.registry;
		tableSet(r, idx, Value((int64_t)S.freeReg));
		S.freeReg = idx;
	}


	// Binary
	[[gnu::always_inline]]
	inline Value _add(const Value& a, const Value& b) {
		double av = assume_number(a);
		double bv = assume_number(b);
		return Value(av + bv);
	}

	[[gnu::always_inline]]
	inline Value _sub(const Value& a, const Value& b) {
		double av = assume_number(a);
		double bv = assume_number(b);
		return Value(av - bv);
	}


	[[gnu::always_inline]]
	inline Value _mul(const Value& a, const Value& b) {
		double av = assume_number(a);
		double bv = assume_number(b);
		return Value(av * bv);
	}

	[[gnu::always_inline]]
	inline Value _div(const Value& a, const Value& b) {
		double av = assume_number(a);
		double bv = assume_number(b);
		return Value(av / bv);
	}


	// Binary Bitwise
	[[gnu::always_inline]]
	inline Value _eq(const Value& a, const Value& b) {

		if (a.type == b.type) {
			switch (a.type) {
			case Type::NIL:  return Value(true);
			case Type::INT:  return Value(a.i == b.i);
			case Type::NUM:  return Value(a.n == b.n);
			case Type::BOOL: return Value(a.b == b.b);
			case Type::OBJ:  if (a.o->type == ObjType::STRING && b.o->type == ObjType::STRING)
				return Value(a.o == b.o);
			}
		}

		// INT vs NUM
		if ((a.type == Type::INT && b.type == Type::NUM) ||
			(a.type == Type::NUM && b.type == Type::INT)) {

			double av = (a.type == Type::INT) ? (double)a.i : a.n;
			double bv = (b.type == Type::INT) ? (double)b.i : b.n;
			return Value(av == bv);
		}

		return Value(false);
	}

	[[gnu::always_inline]]
	inline Value _lt(const Value& a, const Value& b) {

		// number < number
		if ((a.type == Type::INT || a.type == Type::NUM) &&
			(b.type == Type::INT || b.type == Type::NUM)) {

			double av = (a.type == Type::INT) ? (double)a.i : a.n;
			double bv = (b.type == Type::INT) ? (double)b.i : b.n;
			return Value(av < bv);
		}

		// string < string
		if (a.type == Type::OBJ && b.type == Type::OBJ &&
			a.o->type == ObjType::STRING &&
			b.o->type == ObjType::STRING) {

			auto* sa = static_cast<ObjString*>(a.o);
			auto* sb = static_cast<ObjString*>(b.o);
			return Value(compareStrings(sa, sb) < 0);
		}

		luaError("attempt to compare wrong datatypes");
	}

	[[gnu::always_inline]]
	inline Value _gt(const Value& a, const Value& b) {
		return _lt(b, a);
	}

	[[gnu::always_inline]]
	inline Value _le(const Value& a, const Value& b) {
		Value lt = _lt(a, b);
		if (lt.type == Type::BOOL && lt.b)
			return Value(true);
		Value eq = _eq(a, b);
		return eq;
	}

	[[gnu::always_inline]]
	inline Value _ge(const Value& a, const Value& b) {
		Value lt = _lt(b, a);
		if (lt.type == Type::BOOL && lt.b)
			return Value(true);
		Value eq = _eq(a, b);
		return eq;
	}
}




namespace lua {
	// Forward declartion
	struct local;
	struct table_slot;
	inline local make_local(const tinylc::Value& v);



	struct table_slot {
		tinylc::ObjTable* table;
		tinylc::Value key;

		table_slot(tinylc::ObjTable* t, const tinylc::Value& k)
			: table(t), key(k) {
		}

		operator local() const;                 // declaration only
		table_slot& operator=(const local& v);  // declaration only
	};



	struct local {
		tinylc::Value v;
		int32_t rootIndex;

		inline const tinylc::Value& value() const { return v; }
		friend inline lua::local make_local(const tinylc::Value& v);
		inline void growHash(tinylc::ObjTable* t);


		// Helper to register
		void _register() noexcept {
			if (!tinylc::G) return;
			if (v.type != tinylc::Type::OBJ) return;

			auto& S = tinylc::runtime();

			if (S.rootTop >= S.rootCap)
				tinylc::growRoots(S);

			rootIndex = S.rootTop;
			S.roots[S.rootTop++] = &v;
		}

		void _unregister() noexcept {
			if (!tinylc::G) return;
			if (rootIndex < 0) return;

			auto& S = tinylc::runtime();

			if (S.rootTop == 0) {
				rootIndex = -1;
				return;
			}

			uint32_t last = S.rootTop - 1;
			S.rootTop--;

			if ((uint32_t)rootIndex != last) {
				S.roots[rootIndex] = S.roots[last];

				lua::local* moved =
					reinterpret_cast<lua::local*>(
						(char*)S.roots[rootIndex] - offsetof(lua::local, v)
						);

				moved->rootIndex = rootIndex;
			}

			rootIndex = -1;
		}

		local() : v(), rootIndex(-1) {}

		local(int i) noexcept : v((int64_t)i), rootIndex(-1) {}
		local(int64_t i) noexcept : v(i), rootIndex(-1) {}
		local(double n) noexcept : v(n), rootIndex(-1) {}
		explicit local(bool b) noexcept : v(b), rootIndex(-1) {}

		local(const char* s) : v(tinylc::makeString(s)), rootIndex(-1) {
			_register();  // ← strings need GC protection
		}

		local(const local& other) : v(other.v), rootIndex(-1) {
			if (rootIndex >= 0)
				_register();
		}

		local& operator=(const local& other) {
			if (this == &other) return *this;

			// Unregister if we were tracking an object
			if (v.type == tinylc::Type::OBJ )
				_unregister();

			v = other.v;

			// Register if new value is an object
			if (rootIndex >= 0)
				_register();

			return *this;
		}

		local(local&& other) noexcept : v(other.v), rootIndex(-1) {
			if (rootIndex >= 0)
				_register();
		}

		local& operator=(local&& other) noexcept {
			if (this == &other) return *this;

			if (v.type == tinylc::Type::OBJ )
				_unregister();

			v = other.v;

			if (rootIndex >= 0)
				_register();

			return *this;
		}

		~local() noexcept {
			_unregister();
		}



		[[gnu::always_inline]]
		inline local operator+(const local& rhs) const {
			return fromValue(tinylc::_add(v, rhs.v));
		}

		[[gnu::always_inline]]
		inline local operator-(const local& rhs) const {
			return fromValue(tinylc::_sub(v, rhs.v));
		}

		[[gnu::always_inline]]
		inline local operator*(const local& rhs) const {
			return fromValue(tinylc::_mul(v, rhs.v));
		}

		[[gnu::always_inline]]
		inline local operator/(const local& rhs) const {
			return fromValue(tinylc::_div(v, rhs.v));
		}



		[[gnu::always_inline]]
		inline local& operator+=(int64_t rhs) {
			if (v.type == tinylc::Type::INT) {
				v.i += rhs;
				return *this;
			}
			// recover: try to get a number back
			double n;
			if (tinylc::toNumber(v, n)) {
				int64_t as_int = (int64_t)n + rhs;
				if (v.type == tinylc::Type::OBJ ) _unregister();
				v = tinylc::Value(as_int);
				return *this;
			}
			tinylc::arithmeticError();
		}

		[[gnu::always_inline]]
		inline local& operator+=(const local& rhs) {
			v = tinylc::_add(v, rhs.v);
			return *this;
		}

		[[gnu::always_inline]]
		inline local& operator-=(int64_t rhs) {
			if (v.type == tinylc::Type::INT) {
				v.i -= rhs;
				return *this;
			}
			// recover: try to get a number back
			double n;
			if (tinylc::toNumber(v, n)) {
				int64_t as_int = (int64_t)n - rhs;
				if (v.type == tinylc::Type::OBJ ) _unregister();
				v = tinylc::Value(as_int);
				return *this;
			}
			tinylc::arithmeticError();
		}

		[[gnu::always_inline]]
		inline local& operator-=(const local& rhs) {
			v = tinylc::_sub(v, rhs.v);
			return *this;
		}

		[[gnu::always_inline]]
		inline local& operator*=(const local& rhs) {
			v = tinylc::_mul(v, rhs.v);
			return *this;
		}

		[[gnu::always_inline]]
		inline local& operator*=(int64_t rhs) {
			if (v.type == tinylc::Type::INT) {
				v.i *= rhs;
				return *this;
			}

			double n;
			if (tinylc::toNumber(v, n)) {
				if (rootIndex >= 0) _unregister();
				v = tinylc::Value(n * rhs);
				return *this;
			}

			tinylc::arithmeticError();
		}


		[[gnu::always_inline]]
		inline local& operator/=(const local& rhs) {
			v = tinylc::_div(v, rhs.v);
			return *this;
		}



		inline local operator==(const local& rhs) const {
			return fromValue(tinylc::_eq(v, rhs.v));
		}

		inline local operator<(const local& rhs) const {
			return fromValue(tinylc::_lt(v, rhs.v));
		}

		inline local operator>(const local& rhs) const {
			return fromValue(tinylc::_gt(v, rhs.v));
		}

		inline local operator<=(const local& rhs) const {
			return fromValue(tinylc::_le(v, rhs.v));
		}

		inline local operator>=(const local& rhs) const {
			return fromValue(tinylc::_ge(v, rhs.v));
		}
		
		explicit operator bool() const {
		    // Follow Lua logic: only 'nil' and 'false' are falsy
		    return !(v.type == tinylc::Type::NIL || 
		            (v.type == tinylc::Type::BOOL && v.b == false));
		}

		inline local operator[](int64_t index) const {
			if (v.type != tinylc::Type::OBJ ||
				v.o->type != tinylc::ObjType::TABLE) {
				tinylc::luaError("attempt to index a non-table value");
			}

			auto* a = static_cast<tinylc::ObjTable*>(v.o);
			return make_local(tinylc::tableGet(a, (uint32_t)index));
		}

		inline local operator[](int index) const {
			return (*this)[(int64_t)index];
		}

		inline table_slot operator[](const char* key) const {
			auto* a = static_cast<tinylc::ObjTable*>(v.o);
			return table_slot(a, tinylc::makeString(key));
		}

		inline table_slot operator[](const local& key) const {
			auto* a = static_cast<tinylc::ObjTable*>(v.o);
			return table_slot(a, key.value());
		}
	private:
		static inline local fromValue(const tinylc::Value& val) {
			local r;
			r.v = val;
			if (val.type == tinylc::Type::OBJ)
				r._register();
			return r;
		}
	};


	// variables
	const inline lua::local nil{};



	// Table
	inline void growHash(tinylc::ObjTable* t) {
		using namespace tinylc;

		State& S = runtime();
		ObjTableHash* old = tableHash(t);

		uint32_t newCap = old->cap * 2;

		ObjTableHash* h =
			(ObjTableHash*)allocate(S, sizeof(ObjTableHash));

		h->cap = newCap;
		h->count = 0;
		h->data = (HashEntry*)allocate(S, sizeof(HashEntry) * newCap);

		for (uint32_t i = 0; i < newCap; ++i) {
			h->data[i].key = Value();
			h->data[i].value = Value();
		}

		// Reinsert
		for (uint32_t i = 0; i < old->cap; ++i) {
			HashEntry& e = old->data[i];
			if (e.key.type != Type::NIL) {
				uint32_t mask = newCap - 1;
				uint32_t idx = hashValue(e.key) & mask;

				while (h->data[idx].key.type != Type::NIL)
					idx = (idx + 1) & mask;

				h->data[idx] = e;
				h->count++;
			}
		}

		deallocate(S, old->data, sizeof(HashEntry) * old->cap);
		deallocate(S, old, sizeof(ObjTableHash));

		t->hash = h;
	}

	inline table_slot::operator local() const {
		// No hash? → nil
		if (!table->hash)
			return lua::nil;

		auto* slot = tinylc::hashFind(table, key);

		if (slot->key.type == tinylc::Type::NIL)
			return lua::nil;

		return make_local(slot->value);
	}

	inline table_slot& table_slot::operator=(const local& v) {
		tinylc::ensureHash(table);

		auto* h = tinylc::tableHash(table);

		// 🔴 REQUIRED: grow BEFORE find
		if (h->count + 1 > h->cap * 0.75)
			lua::growHash(table);

		auto* slot = tinylc::hashFind(table, key);

		if (slot->key.type == tinylc::Type::NIL)
			h->count++;

		slot->key = key;
		slot->value = v.value();

		return *this;
	}


	struct entry {
		bool isKV;
		tinylc::Value key;
		tinylc::Value value;

		template <typename V>
		entry(V&& v)
			: isKV(false),
			key(),
			value(local(std::forward<V>(v)).value()) {
		}

		entry(const local& k, const local& v)
			: isKV(true),
			key(k.value()),
			value(v.value()) {
		}
	};

	inline entry kv(const local& k, const local& v) {
		return entry(k, v);
	}

	template <typename K, typename V>
	inline entry kv(K&& k, V&& v) {
		return entry(
			local(std::forward<K>(k)),
			local(std::forward<V>(v))
		);
	}


	// Local exposing
	inline lua::local make_local(const tinylc::Value& v) {
		lua::local tmp;
		tmp.v = v;
		return tmp;
	}


	// Init
	inline void init(void) {
		tinylc::init();
	}


	// Standard library
	inline local tonumber(const local& x) {
		double n;
		if (tinylc::toNumber(x.value(), n)) {
			return make_local(tinylc::Value(n));
		}
		return make_local(tinylc::Value());
	}

	inline local tostring(const local& x) {
		tinylc::Value out = tinylc::toString(x.value());
		return make_local(out);
	}

	inline bool toboolean(const lua::local& v) {
		const tinylc::Value& x = v.value();
		return !(x.type == tinylc::Type::NIL ||
			(x.type == tinylc::Type::BOOL && x.b == false));
	}


	inline void print() {
		std::cout << std::endl;
	}

	template <typename T, typename... Rest>
	inline void print(const T& first, const Rest&... rest) {
		tinylc::_printValue(local(first).value());
		if constexpr (sizeof...(rest) > 0) {
			std::cout << " ";
			print(rest...);
		}
		else {
			std::cout << std::endl;
		}
	}

	inline local type(const local& x) {
		using namespace tinylc;

		const Value& v = x.value();

		if (v.type != Type::OBJ) {
			return local(typeName(v.type));
		}

		// Object types
		switch (v.o->type) {
		case ObjType::STRING: return local("string");
		case ObjType::TABLE:  return local("table");
			// case ObjType::FUNCTION: return local("function");
			// case ObjType::THREA:   return local("thread");
		}

		return local("object");
	}

	inline local assert(const local& cond, const local& msg = local()) {
		if (!toboolean(cond)) {
			if (msg.value().type == tinylc::Type::NIL)
				tinylc::luaError("assertion failed!");
			else {
				local s = tostring(msg);
				tinylc::luaError(
					static_cast<tinylc::ObjString*>(s.value().o)->chars
				);
			}
		}
		return cond;
	}



	inline local collectgarbage(const char* opt = "collect") {
		auto& S = tinylc::runtime();
		if (!std::strcmp(opt, "collect")) tinylc::collectGarbage(S);
		if (!std::strcmp(opt, "count")) return local((double)S.bytesAllocated / 1024.0);
		return nil;
	}


	inline local table(void) {
		lua::local out;
		out.v = tinylc::makeTable();
		out._register();
		return out;
	}


	inline local table(std::initializer_list<entry> init) {
		lua::local out;
		out.v = tinylc::makeTable();
		out._register();

		auto* t = static_cast<tinylc::ObjTable*>(out.value().o);

		// Pre-count hash entries
		uint32_t hashCount = 0;
		for (const entry& e : init) {
			if (e.isKV) hashCount++;
		}

		// Pre-allocate hash if needed
		if (hashCount > 0) {
			tinylc::ensureHash(t);
			auto* h = tinylc::tableHash(t);

			// Resize hash upfront to avoid rehashing
			while (h->cap < hashCount * 2) {
				// ... grow hash ...
			}
		}

		uint32_t arrayIndex = 0;
		for (const entry& e : init) {
			if (!e.isKV) {
				tinylc::tableSet(t, arrayIndex++, e.value);
			}
			else {
				auto* h = tinylc::tableHash(t);
				auto* slot = tinylc::hashFind(t, e.key);

				if (slot->key.type == tinylc::Type::NIL)
					h->count++;

				slot->key = e.key;
				slot->value = e.value;
			}
		}

		return out;
	}


	inline local get(const local& arr, int64_t index) {
		auto* a = static_cast<tinylc::ObjTable*>(arr.value().o);
		return make_local(tableGet(a, (uint32_t)index));
	}

	inline void set(const local& arr, int64_t index, const local& v) {
		auto* a = static_cast<tinylc::ObjTable*>(arr.value().o);
		tableSet(a, (uint32_t)index, v.value());
	}
}






int main(void) {
	lua::init();

	lua::local x = lua::nil,b = 12;	
	lua::print(x,b);	
	
	if (x == lua::nil) {
		lua::print("X is nil!");
	}
	
	lua::local type_of_b = lua::type(b);
	if (type_of_b == "number") {
		lua::print("B is a number!");
	} else {
		lua::print("B is not a number!");
		lua::print("B is a", type_of_b);
	}

	return 0;
}
