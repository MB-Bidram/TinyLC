#include "table.hpp"

namespace tinylc {
	// Table
	void markTable(State& S, Obj* obj) {
		auto* t = (ObjTable*)obj;

		// array
		if (t->kind == ObjTable::ArrayKind::MIXED) {
			for (uint32_t i = 0; i < t->size; ++i) {
				if (t->values[i].type == Type::OBJ) {
					markObject(S, t->values[i].o);
				}
			}
		}

		// hash
		if (t->hash) {
			ObjTableHash* h = tableHash(t);
			for (uint32_t i = 0; i < h->cap; ++i) {
				if (h->data[i].key.type == Type::OBJ) {
					markObject(S, h->data[i].key.o);
				}
				if (h->data[i].value.type == Type::OBJ) {
					markObject(S, h->data[i].value.o);
				}
			}
		}
	}

	void freeTable(State& S, Obj* obj) {
		auto* t = (ObjTable*)obj;

		auto kind = t->kind;
		auto capacity = t->capacity;
		auto hash = t->hash;

		void* arrayPtr = nullptr;
		size_t elemSize = 0;

		switch (kind) {
		case ObjTable::ArrayKind::INT: {
			arrayPtr = t->ints;
			elemSize = sizeof(int64_t);
			break;
		};
		case ObjTable::ArrayKind::NUM: {
			arrayPtr = t->nums;
			elemSize = sizeof(double);
			break;
		};
		case ObjTable::ArrayKind::MIXED: {
			arrayPtr = t->values;
			elemSize = sizeof(Value);
			break;
		};

		default: {
			printf("Unhandled *::ArrayKind:: (Int)%d\n", kind);
			std::abort();
		};
		}

		if (hash) {
			ObjTableHash* h = (ObjTableHash*)hash;
			deallocate(S, h->data, sizeof(HashEntry) * h->cap);
			deallocate(S, h, sizeof(ObjTableHash));
		}

		if (arrayPtr) {
			deallocate(S, arrayPtr, elemSize * capacity);
		};

		deallocate(S, t, sizeof(ObjTable));
	}

	ObjTable* newTable(uint32_t cap) {
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

	void promoteIntToNum(ObjTable* a) {
		State& S = runtime();

		double* p = (double*)allocate(S, sizeof(double) * a->capacity);
		for (uint32_t i = 0; i < a->size; ++i) {
			p[i] = (double)a->ints[i];
		}

		deallocate(S, a->ints, sizeof(int64_t) * a->capacity);
		a->nums = p;
		a->kind = ObjTable::ArrayKind::NUM;
	}

	void promoteToMixed(ObjTable* a) {
		State& S = runtime();

		Value* p = (Value*)allocate(S, sizeof(Value) * a->capacity);

		if (a->kind == ObjTable::ArrayKind::INT) {
			for (uint32_t i = 0; i < a->size; ++i) {
				p[i] = Value(a->ints[i]);
			}
			deallocate(S, a->ints, sizeof(int64_t) * a->capacity);
		} else {
			for (uint32_t i = 0; i < a->size; ++i) {
				p[i] = Value(a->nums[i]);
			}
			deallocate(S, a->nums, sizeof(double) * a->capacity);
		}

		for (uint32_t i = a->size; i < a->capacity; ++i) {
			p[i] = Value();
		}

		a->values = p;
		a->kind = ObjTable::ArrayKind::MIXED;
	}


	void tableEnsure(ObjTable* a, uint32_t index) {
		if (index < a->capacity) {
			return;
		}

		uint32_t newCap = a->capacity;
		while (newCap <= index) {
			newCap *= 2;
		}

		State& S = runtime();

		switch (a->kind) {
		case ObjTable::ArrayKind::INT: {
			int64_t* p = (int64_t*)allocate(S, sizeof(int64_t) * newCap);
			std::memcpy(p, a->ints, sizeof(int64_t) * a->capacity);
			std::memset(p + a->capacity, 0, sizeof(int64_t) * (newCap - a->capacity));
			deallocate(S, a->ints, sizeof(int64_t) * a->capacity);
			a->ints = p;
			break;
		};

		case ObjTable::ArrayKind::NUM: {
			double* p = (double*)allocate(S, sizeof(double) * newCap);
			std::memcpy(p, a->nums, sizeof(double) * a->capacity);
			std::memset(p + a->capacity, 0, sizeof(double) * (newCap - a->capacity));
			deallocate(S, a->nums, sizeof(double) * a->capacity);
			a->nums = p;
			break;
		};

		case ObjTable::ArrayKind::MIXED: {
			Value* p = (Value*)allocate(S, sizeof(Value) * newCap);
			std::memcpy(p, a->values, sizeof(Value) * a->capacity);
			for (uint32_t i = a->capacity; i < newCap; ++i) {
				p[i] = Value();
			}
			deallocate(S, a->values, sizeof(Value) * a->capacity);
			a->values = p;
			break;
		};
		}

		a->capacity = newCap;
	}

	Value tableGet(ObjTable* a, uint32_t i) {
		if (i >= a->size) { return Value(); } // nil
		return tableGetRaw(a, i);
	}

	void tableSet(ObjTable* a, uint32_t index, const Value& v) {
		tableEnsure(a, index);

		switch (a->kind) {
			case ObjTable::ArrayKind::INT: {
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
			};
			case ObjTable::ArrayKind::NUM: {
				if (v.type == Type::INT || v.type == Type::NUM) {
					a->nums[index] = (v.type == Type::INT) ? (double)v.i : v.n;
					break;
				}
				promoteToMixed(a);
				a->values[index] = v;
				break;
			};
			case ObjTable::ArrayKind::MIXED: {
				a->values[index] = v;
				break;
			};
		}

		if (index >= a->size) { a->size = index + 1; };
	}
} // namespace tinylc