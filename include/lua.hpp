#pragma once

#include <cstdint>
#include <initializer_list>
#include <utility>

#include "src/gc.hpp"
#include "src/operator.hpp"
#include "src/stdlib.hpp"
#include "src/string.hpp"
#include "src/table.hpp"

namespace lua {
	// Forward declartion
	struct local;
	struct table_slot;

	// Local exposure
	local make_local(const tinylc::Value& v);

	struct table_slot {
		tinylc::ObjTable* table;
		tinylc::Value key;

		table_slot(tinylc::ObjTable* t, const tinylc::Value& k)
			: table(t)
			, key(k) {}

		operator local() const; // declaration only
		table_slot& operator=(const local& v); // declaration only
	};

	struct local {
		tinylc::Value v;
		int32_t rootIndex;

		const tinylc::Value& value() const { return v; }

		// scalars 'never' publish their addresses to the runtime
		void _register() noexcept {
			if (v.type == tinylc::Type::OBJ && rootIndex < 0)
				_registerObject();
		}

		void _unregister() noexcept {
			const int32_t index = rootIndex;
			if (index >= 0)
				_unregisterObject(index);
			rootIndex = -1;
		}

		local() noexcept
			: v()
			, rootIndex(-1) {}
		local(int i) noexcept
			: v(int64_t(i))
			, rootIndex(-1) {}
		local(int64_t i) noexcept
			: v(i)
			, rootIndex(-1) {}
		local(double n) noexcept
			: v(n)
			, rootIndex(-1) {}
		explicit local(bool b) noexcept
			: v(b)
			, rootIndex(-1) {}
		local(const char* s);
		local(const local& other)
			: v(other.v)
			, rootIndex(-1) {
			_register();
		}
		local& operator=(const local& other) {
			return this == &other ? *this : assignValue(other.v);
		}
		local(local&& other) noexcept
			: v(other.v)
			, rootIndex(-1) {
			_register();
		}
		local& operator=(local&& other) noexcept {
			return operator=(static_cast<const local&>(other));
		}

		~local() noexcept { _unregister(); }
		local operator+(const local& rhs) const { return local(tinylc::_add(v, rhs.v).n); }
		local operator-(const local& rhs) const { return local(tinylc::_sub(v, rhs.v).n); }
		local operator*(const local& rhs) const { return local(tinylc::_mul(v, rhs.v).n); }
		local operator/(const local& rhs) const { return local(tinylc::_div(v, rhs.v).n); }
		local& operator+=(int64_t rhs) {
			if (v.type == tinylc::Type::INT) {
				v.i = tinylc::integerFromBits(uint64_t(v.i) + uint64_t(rhs));
				return *this;
			}
			return assignValue(tinylc::_add(v, tinylc::Value(rhs)));
		}
		local& operator+=(const local& rhs) { return assignValue(tinylc::_add(v, rhs.v)); }
		local& operator-=(int64_t rhs) {
			if (v.type == tinylc::Type::INT) {
				v.i = tinylc::integerFromBits(uint64_t(v.i) - uint64_t(rhs));
				return *this;
			}
			return assignValue(tinylc::_sub(v, tinylc::Value(rhs)));
		}
		local& operator-=(const local& rhs) { return assignValue(tinylc::_sub(v, rhs.v)); }
		local& operator*=(const local& rhs) { return assignValue(tinylc::_mul(v, rhs.v)); }
		local& operator*=(int64_t rhs) {
			if (v.type == tinylc::Type::INT) {
				v.i = tinylc::integerFromBits(uint64_t(v.i) * uint64_t(rhs));
				return *this;
			}
			return assignValue(tinylc::_mul(v, tinylc::Value(rhs)));
		}
		local& operator/=(const local& rhs) { return assignValue(tinylc::_div(v, rhs.v)); }
		local operator==(const local& rhs) const { return local(tinylc::_eq(v, rhs.v).b); }
		local operator<(const local& rhs) const { return local(tinylc::_lt(v, rhs.v).b); }
		local operator>(const local& rhs) const { return local(tinylc::_gt(v, rhs.v).b); }
		local operator<=(const local& rhs) const { return local(tinylc::_le(v, rhs.v).b); }
		local operator>=(const local& rhs) const { return local(tinylc::_ge(v, rhs.v).b); }

		explicit operator bool() const {
			return !(v.type == tinylc::Type::NIL || (v.type == tinylc::Type::BOOL && !v.b));
		}
		local operator[](int64_t index) const;
		local operator[](int index) const;
		table_slot operator[](const char* key) const;
		table_slot operator[](const local& key) const;

	private:
		void _registerObject() noexcept;
		static void _unregisterObject(int32_t index) noexcept;

		local& assignValue(tinylc::Value val) {
			_unregister();
			v = val;
			_register();
			return *this;
		}
	};

	// Variables
	inline const lua::local nil{};

	// Table
	void growHash(tinylc::ObjTable* t);

	struct entry {
		bool isKV;
		tinylc::Value key;
		tinylc::Value value;

		template <typename V>
		entry(V&& v)
			: isKV(false)
			, key()
			, value(local(std::forward<V>(v)).value()) {}

		entry(const local& k, const local& v)
			: isKV(true)
			, key(k.value())
			, value(v.value()) {}
	};

	entry kv(const local& k, const local& v);

	template <typename K, typename V> entry kv(K&& k, V&& v);

	// Init (wrap)
	void init(void);

	// Standard library
	local tonumber(const local& x);
	local tostring(const local& x);
	bool toboolean(const lua::local& v);
	void print();

	template <typename T, typename... Rest> inline void print(const T& first, const Rest&... rest) {
		tinylc::_printValue(local(first).value());
		if constexpr (sizeof...(rest) > 0) {
			std::cout << " ";
			print(rest...);
		} else {
			std::cout << std::endl;
		}
	}

	local type(const local& x);
	local assert(const local& cond, const local& msg = local());
	local collectgarbage(const char* opt = "collect");
	local table(void);
	local table(std::initializer_list<entry> init);
	local get(const local& arr, int64_t index);
	void set(const local& arr, int64_t index, const local& v);
} // namespace lua
