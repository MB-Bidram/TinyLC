#include <cstdint>
#include <initializer_list>
#include <utility>

#include "gc.hpp"
#include "stdlib.hpp"
#include "table.hpp"
#include "string.hpp"
#include "operator.hpp"

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
			: table(t), key(k) {
		}

		operator local() const;                 // declaration only
		table_slot& operator=(const local& v);  // declaration only
	};



	struct local {
		tinylc::Value v;
		int32_t rootIndex;

		const tinylc::Value& value() const;

		// Helper to register
		void _register() noexcept;

		void _unregister() noexcept;

		local();
		local(int i) noexcept;
		local(int64_t i) noexcept;
		local(double n) noexcept;
		explicit local(bool b) noexcept;
		local(const char* s);
		local(const local& other);
		local& operator=(const local& other);
		local(local&& other) noexcept;
		local& operator=(local&& other) noexcept;

		~local() noexcept;
		local operator+(const local& rhs) const;
		local operator-(const local& rhs) const;
		local operator*(const local& rhs) const;
		local operator/(const local& rhs) const;
		local& operator+=(int64_t rhs);
		local& operator+=(const local& rhs);
		local& operator-=(int64_t rhs);
		local& operator-=(const local& rhs);
		local& operator*=(const local& rhs);
		local& operator*=(int64_t rhs);
		local& operator/=(const local& rhs);
		local operator==(const local& rhs) const;
		local operator<(const local& rhs) const;
		local operator>(const local& rhs) const;
		local operator<=(const local& rhs) const;
		local operator>=(const local& rhs) const;
		
		explicit operator bool() const;
		local operator[](int64_t index) const;
		local operator[](int index) const;
		table_slot operator[](const char* key) const;
		table_slot operator[](const local& key) const;
	private:
		static local fromValue(const tinylc::Value& val);
	};


	// variables
	const lua::local nil{}; // nil

	// Table
	void growHash(tinylc::ObjTable* t);


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

	entry kv(const local& k, const local& v);

	template <typename K, typename V>
	entry kv(K&& k, V&& v);

	// Init (wrap)
	void init(void);

	// Standard library
	local tonumber(const local& x);
	local tostring(const local& x);
	bool toboolean(const lua::local& v);
	void print();

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
	
	local type(const local& x);
	local assert(const local& cond, const local& msg = local());
	local collectgarbage(const char* opt = "collect");
	local table(void);
	local table(std::initializer_list<entry> init);
	local get(const local& arr, int64_t index);
	void set(const local& arr, int64_t index, const local& v);
}