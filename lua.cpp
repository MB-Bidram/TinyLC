#include "lua.hpp"

namespace lua {
	const tinylc::Value& local::value() const {
		return v;
	}

	// Helper to register
	void local::_register() noexcept {
		if (!tinylc::G)
			return;
		if (v.type != tinylc::Type::OBJ)
			return;
		if (rootIndex >= 0)
			return;

		auto& S = tinylc::runtime();

		if (S.rootTop >= S.rootCap) {
			tinylc::growRoots(S);
		}

		rootIndex = S.rootTop;
		S.roots[S.rootTop++] = &v;
	}

	void local::_unregister() noexcept {
		if (!tinylc::G)
			return;
		if (rootIndex < 0)
			return;

		auto& S = tinylc::runtime();

		if (S.rootTop == 0) {
			rootIndex = -1;
			return;
		}

		uint32_t last = S.rootTop - 1;
		S.rootTop--;

		if ((uint32_t)rootIndex != last) {
			S.roots[rootIndex] = S.roots[last];

			lua::local* moved = reinterpret_cast<lua::local*>((char*)S.roots[rootIndex] -
															  offsetof(lua::local, v));

			moved->rootIndex = rootIndex;
		}

		rootIndex = -1;
	}

	local::local()
		: v()
		, rootIndex(-1) {}

	local::local(int i) noexcept
		: v((int64_t)i)
		, rootIndex(-1) {}

	local::local(int64_t i) noexcept
		: v(i)
		, rootIndex(-1) {}

	local::local(double n) noexcept
		: v(n)
		, rootIndex(-1) {}

	local::local(bool b) noexcept
		: v(b)
		, rootIndex(-1) {}

	local::local(const char* s)
		: v(tinylc::makeString(s))
		, rootIndex(-1) {
		_register(); // gc protection for strings
	}

	local::local(const local& other)
		: v(other.v)
		, rootIndex(-1) {
		_register();
	}

	local& local::operator=(const local& other) {
		if (this == &other)
			return *this;

		// Unregister if we were tracking an object
		_unregister();
		v = other.v;

		// Register if new value is an object
		_register();

		return *this;
	}

	local::local(local&& other) noexcept
		: v(other.v)
		, rootIndex(-1) {
		_register();
	}

	local& local::operator=(local&& other) noexcept {
		if (this == &other)
			return *this;

		_unregister();
		v = other.v;
		_register();

		return *this;
	}

	local::~local() noexcept {
		_unregister();
	}


	local local::operator+(const local& rhs) const {
		return fromValue(tinylc::_add(v, rhs.v));
	}

	local local::operator-(const local& rhs) const {
		return fromValue(tinylc::_sub(v, rhs.v));
	}

	local local::operator*(const local& rhs) const {
		return fromValue(tinylc::_mul(v, rhs.v));
	}

	local local::operator/(const local& rhs) const {
		return fromValue(tinylc::_div(v, rhs.v));
	}

	local& local::operator+=(int64_t rhs) {
		if (v.type == tinylc::Type::INT) {
			v.i += rhs;
			return *this;
		}

		double n;
		if (tinylc::toNumber(v, n)) {
			int64_t as_int = (int64_t)n + rhs;
			_unregister();
			v = tinylc::Value(as_int);
			return *this;
		}
		tinylc::arithmeticError();
		return *this;
	}

	local& local::operator+=(const local& rhs) {
		v = tinylc::_add(v, rhs.v);
		_register();
		return *this;
	}

	local& local::operator-=(int64_t rhs) {
		if (v.type == tinylc::Type::INT) {
			v.i -= rhs;
			return *this;
		}

		double n;
		if (tinylc::toNumber(v, n)) {
			int64_t as_int = (int64_t)n - rhs;
			_unregister();
			v = tinylc::Value(as_int);
			return *this;
		}
		tinylc::arithmeticError();
		return *this;
	}

	local& local::operator-=(const local& rhs) {
		v = tinylc::_sub(v, rhs.v);
		_register();
		return *this;
	}

	local& local::operator*=(const local& rhs) {
		v = tinylc::_mul(v, rhs.v);
		_register();
		return *this;
	}

	local& local::operator*=(int64_t rhs) {
		if (v.type == tinylc::Type::INT) {
			v.i *= rhs;
			return *this;
		}

		double n;
		if (tinylc::toNumber(v, n)) {
			_unregister();
			v = tinylc::Value(n * rhs);
			return *this;
		}

		tinylc::arithmeticError();
		return *this;
	}


	local& local::operator/=(const local& rhs) {
		v = tinylc::_div(v, rhs.v);
		_register();
		return *this;
	}


	local local::operator==(const local& rhs) const { return fromValue(tinylc::_eq(v, rhs.v)); }
	local local::operator<(const local& rhs) const { return fromValue(tinylc::_lt(v, rhs.v)); }
	local local::operator>(const local& rhs) const { return fromValue(tinylc::_gt(v, rhs.v)); }
	local local::operator<=(const local& rhs) const { return fromValue(tinylc::_le(v, rhs.v)); }
	local local::operator>=(const local& rhs) const { return fromValue(tinylc::_ge(v, rhs.v)); }

	local::operator bool() const {
		return !(v.type == tinylc::Type::NIL || (v.type == tinylc::Type::BOOL && v.b == false));
	}

	local local::operator[](int64_t index) const {
		if (v.type != tinylc::Type::OBJ || v.o->type != tinylc::ObjType::TABLE) {
			tinylc::luaError("attempt to index a non-table value");
		}

		auto* a = static_cast<tinylc::ObjTable*>(v.o);
		return make_local(tinylc::tableGet(a, (uint32_t)index));
	}

	local local::operator[](int index) const { return (*this)[(int64_t)index]; }

	table_slot local::operator[](const char* key) const {
		auto* a = static_cast<tinylc::ObjTable*>(v.o);
		return table_slot(a, tinylc::makeString(key));
	}

	table_slot local::operator[](const local& key) const {
		auto* a = static_cast<tinylc::ObjTable*>(v.o);
		return table_slot(a, key.value());
	}

	local local::fromValue(const tinylc::Value& val) {
		local r;
		r.v = val;
		r._register();
		return r;
	}


	// Table
	void growHash(tinylc::ObjTable* t) {
		using namespace tinylc;

		State& S = runtime();
		ObjTableHash* old = tableHash(t);

		uint32_t newCap = old->cap * 2;

		ObjTableHash* h = (ObjTableHash*)allocate(S, sizeof(ObjTableHash));

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

				while (h->data[idx].key.type != Type::NIL) {
					idx = (idx + 1) & mask;
				}

				h->data[idx] = e;
				h->count++;
			}
		}

		deallocate(S, old->data, sizeof(HashEntry) * old->cap);
		deallocate(S, old, sizeof(ObjTableHash));

		t->hash = h;
	}

	table_slot::operator local() const {
		if (!table->hash) {
			return lua::nil;
		}

		auto* slot = tinylc::hashFind(table, key);

		if (slot->key.type == tinylc::Type::NIL) {
			return lua::nil;
		}

		return make_local(slot->value);
	}

	table_slot& table_slot::operator=(const local& v) {
		tinylc::ensureHash(table);
		auto* h = tinylc::tableHash(table);

		auto* slot = tinylc::hashFind(table, key);
		if (slot->key.type == tinylc::Type::NIL && h->count + 1 > h->cap * 0.75) {
			lua::growHash(table);
			h = tinylc::tableHash(table);
			slot = tinylc::hashFind(table, key);
		}

		if (slot->key.type == tinylc::Type::NIL) {
			h->count++;
		}

		slot->key = key;
		slot->value = v.value();

		return *this;
	}


	entry kv(const local& k, const local& v) {
		return entry(k, v);
	}


	// Local exposing
	lua::local make_local(const tinylc::Value& v) {
		lua::local tmp;
		tmp.v = v;
		tmp._register();
		return tmp;
	}


	// Init (wrap)
	void init(void) {
		tinylc::init();
	}


	// Standard library
	local tonumber(const local& x) {
		double n;
		if (tinylc::toNumber(x.value(), n)) {
			return make_local(tinylc::Value(n));
		}
		return make_local(tinylc::Value());
	}

	local tostring(const local& x) {
		tinylc::Value out = tinylc::toString(x.value());
		return make_local(out);
	}

	bool toboolean(const lua::local& v) {
		const tinylc::Value& x = v.value();
		return !(x.type == tinylc::Type::NIL || (x.type == tinylc::Type::BOOL && x.b == false));
	}

	inline void print() { std::cout << std::endl; }

	local type(const local& x) {
		using namespace tinylc;

		const Value& v = x.value();

		if (v.type != Type::OBJ) {
			return local(typeName(v.type));
		}

		// Object types
		switch (v.o->type) {
		case ObjType::STRING:
			return local("string");
		case ObjType::TABLE:
			return local("table");
			// case ObjType::FUNCTION: return local("function");
			// case ObjType::THREA:   return local("thread");
		}

		return local("object");
	}

	local assert(const local& cond, const local& msg) {
		if (!toboolean(cond)) {
			if (msg.value().type == tinylc::Type::NIL)
				tinylc::luaError("assertion failed!");
			else {
				local s = tostring(msg);
				tinylc::luaError(static_cast<tinylc::ObjString*>(s.value().o)->chars);
			}
		}
		return cond;
	}


	local collectgarbage(const char* opt) {
		auto& S = tinylc::runtime();
		if (!std::strcmp(opt, "collect"))
			tinylc::collectGarbage(S);
		if (!std::strcmp(opt, "count"))
			return local((double)S.bytesAllocated / 1024.0);
		return nil;
	}


	local table(void) {
		lua::local out;
		out.v = tinylc::makeTable();
		out._register();
		return out;
	}


	local table(std::initializer_list<entry> init) {
		lua::local out;
		out.v = tinylc::makeTable();
		out._register();

		auto* t = static_cast<tinylc::ObjTable*>(out.value().o);

		uint32_t hashCount = 0;
		for (const entry& e : init) {
			if (e.isKV)
				hashCount++;
		}

		if (hashCount > 0) {
			tinylc::ensureHash(t);
			while (tinylc::tableHash(t)->cap < hashCount * 2) {
				growHash(t);
			}
		}

		uint32_t arrayIndex = 0;
		for (const entry& e : init) {
			if (!e.isKV) {
				tinylc::tableSet(t, arrayIndex++, e.value);
			} else {
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


	local get(const local& arr, int64_t index) {
		auto* a = static_cast<tinylc::ObjTable*>(arr.value().o);
		return make_local(tinylc::tableGet(a, (uint32_t)index));
	}

	void set(const local& arr, int64_t index, const local& v) {
		auto* a = static_cast<tinylc::ObjTable*>(arr.value().o);
		tinylc::tableSet(a, (uint32_t)index, v.value());
	}
} // namespace lua