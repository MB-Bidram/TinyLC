#include "gc.hpp"

namespace tinylc {
	void markObject(State&, Obj*);
	void collectGarbage(State& S);

	void growRoots(State& S) {
		uint32_t newCap = S.rootCap * 2;
		S.roots = (Value**)std::realloc(S.roots, sizeof(Value*) * newCap);
		S.rootCap = newCap;
	}

	void maybeCollect(State& S) {
		if (!S.gcEnabled)
			return;

		if (S.bytesAllocated >= S.nextGC) {
			collectGarbage(S);

			// grow threshold
			S.nextGC = S.bytesAllocated * 2 + 1024;
		}
	}

	void markInternTable(State& S) {
		for (size_t i = 0; i < S.capacity; ++i) {
			for (InternNode* node = S.buckets[i]; node; node = node->next)
				markObject(S, (Obj*)node->str);
		}
	}

	// Memory
	void* allocate(State& S, size_t size) {
		S.bytesAllocated += size;
		if (!S.gcRunning && S.gcEnabled && S.bytesAllocated >= S.nextGC)
			maybeCollect(S);
		return xmalloc(size);
	}

	void deallocate(State& S, void* p, size_t size) {
		if (S.bytesAllocated >= size) {
			S.bytesAllocated -= size;
		} else {
			S.bytesAllocated = 0; // Prevent underflow
		}
		std::free(p);
	}

	void removeIntern(State& S, ObjString* victim) {
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

	void* xmalloc(size_t size) {
		void* p = malloc(size);
		if (!p)
			luaError("memory allocation failed");
		return p;
	}

	// Debug
	void debugListObjects(State& S) {
		size_t count = 0;
		for (Obj* o = S.allObjects; o; o = o->next) {
			count++;
			std::cout << "[" << count << "] "
					  << "obj=" << o << " type=" << (int)o->type << " marked=" << (int)o->markEpoch
					  << "\n";
		}
		if (count == 0)
			std::cout << "(no objects)\n";
	}

	// Garbage Collector
	[[gnu::cold]]
	void markObject(State& S, Obj* obj) {
		if (!obj) {
			return;
		};

		if (obj->markEpoch > S.currentEpoch) {
			std::abort();
		}
		if (obj->markEpoch == S.currentEpoch) {
			return;
		};

		obj->markEpoch = S.currentEpoch;

		if (obj->type == ObjType::TABLE) {
			markTable(S, obj);
		};
	}

	void freeString(State& S, Obj* obj) {
		auto* s = (ObjString*)obj;
		removeIntern(S, s);
		deallocate(S, s->chars, s->length + 1);
		deallocate(S, s, sizeof(ObjString));
	}

	void sweep(State& S) {
		Obj** p = &S.allObjects;
		while (*p) {
			Obj* obj = *p;
			if (obj->markEpoch != S.currentEpoch) {
				*p = obj->next;
				obj->free(S, obj);
			} else {
				p = &obj->next;
			}
		}
	}

	[[gnu::cold]]
	void collectGarbage(State& S) {
		// debugListObjects(S);
		S.gcRunning = true;
		S.currentEpoch++;

		markObject(S, (Obj*)S.registry);
		markInternTable(S);

		for (uint32_t i = 0; i < S.rootTop; ++i) {
			Value* v = S.roots[i];
			if (v && v->type == Type::OBJ) {
				markObject(S, v->o);
			}
		}

		sweep(S);

		S.nextGC = S.bytesAllocated * 2 + 1024;
		S.gcRunning = false;
	}

	// Registery
	uint32_t registryAdd(State& S, const Value& v) {
		ObjTable* r = S.registry;

		if (S.freeReg != UINT32_MAX) {
			uint32_t idx = S.freeReg;
			Value next = tableGet(r, idx);
			S.freeReg = (next.type == Type::INT) ? (uint32_t)next.i : UINT32_MAX;

			tableSet(r, idx, v);
			return idx;
		}

		uint32_t idx = r->size;
		tableSet(r, idx, v);
		return idx;
	}

	void registryRemove(State& S, uint32_t idx) {
		ObjTable* r = S.registry;
		tableSet(r, idx, Value((int64_t)S.freeReg));
		S.freeReg = idx;
	}
} // namespace tinylc