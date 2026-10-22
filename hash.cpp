#include "hash.hpp"

namespace tinylc {
	bool keyEqual(const Value& a, const Value& b) {
		if (a.type != b.type) {
			if ((a.type == Type::INT && b.type == Type::NUM) ||
				(a.type == Type::NUM && b.type == Type::INT)) {
				double av = (a.type == Type::INT) ? (double)a.i : a.n;
				double bv = (b.type == Type::INT) ? (double)b.i : b.n;
				return av == bv;
			}
			return false;
		}

		switch (a.type) {
		case Type::INT: {
			return a.i == b.i;
		};
		case Type::NUM: {
			return a.n == b.n;
		};
		case Type::BOOL: {
			return a.b == b.b;
		};
		case Type::OBJ: {
			return a.o == b.o;
		};
		case Type::NIL: {
			return true;
		};
		}
		return false;
	}

	uint32_t hashString(const char* s, size_t len) {
		uint32_t h = 2166136261u;
		for (size_t i = 0; i < len; i++) {
			h ^= (uint8_t)s[i];
			h *= 16777619u;
		}
		return h;
	}

	Value tableGetRaw(ObjTable* a, uint32_t i) {
		if (i >= a->size) {
			return Value();
		}; // safety check

		auto kind = a->kind;
		switch (kind) {
		case ObjTable::ArrayKind::INT: {
			return Value(a->ints[i]);
		};
		case ObjTable::ArrayKind::NUM: {
			return Value(a->nums[i]);
		};
		case ObjTable::ArrayKind::MIXED: {
			return a->values[i];
		};
		}
		return {};
	}

	uint32_t hashValue(const Value& k) {
		switch (k.type) {
		case Type::INT: {
			return (uint32_t)k.i * 2654435761u;
		};
		case Type::NUM: {
			uint64_t u;
			std::memcpy(&u, &k.n, sizeof(u));
			return (uint32_t)(u ^ (u >> 32));
		};
		case Type::OBJ: {
			if (k.o->type == ObjType::STRING) {
				return static_cast<ObjString*>(k.o)->hash;
			};
			return (uint32_t)(uintptr_t)k.o;
		};
		case Type::BOOL: {
			return k.b ? 0xABCDEF01u : 0x12345678u;
		};
		default: {
			return 0;
		};
		}
	}


	void ensureHash(ObjTable* t) {
		if (t->hash) {
			return;
		};

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

	ObjTableHash* tableHash(ObjTable* t) {
		return (ObjTableHash*)t->hash;
	}


	HashEntry* hashFind(ObjTable* t, const Value& key) {
		if (!t->hash) {
			luaError("internal error: hashFind on table without hash");
		};

		ObjTableHash* h = tableHash(t);
		uint32_t mask = h->cap - 1;

		uint32_t i = hashValue(key) & mask;
		for (;;) {
			HashEntry* e = &h->data[i];
			if (e->key.type == Type::NIL || keyEqual(e->key, key)) {
				return e;
			};
			i = (i + 1) & mask;
		}
	}
} // namespace tinylc