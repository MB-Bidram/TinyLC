#include "state.hpp"

namespace tinylc {
	thread_local State* G = nullptr;

	void init() {
		if (!G) {
			G = (State*)xmalloc(sizeof(State));
			new (G) State();

			G->registry = newTable(8);
			G->registry->kind = ObjTable::ArrayKind::MIXED;
			G->registry->values = (Value*)allocate(*G, sizeof(Value) * G->registry->capacity);

			for (uint32_t i = 0; i < G->registry->capacity; ++i)
				G->registry->values[i] = Value();
		}
	}

	State& runtime() {
		if (!G)
			luaError("Lua state not initialized");
		return *G;
	}
} // namespace tinylc