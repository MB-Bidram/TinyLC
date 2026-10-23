#include "operator.hpp"
#include "error.hpp"
#include "string.hpp"

namespace tinylc {
	int compareObjects(Value a, Value b) {
		if (a.type == Type::OBJ && b.type == Type::OBJ && a.o->type == ObjType::STRING &&
			b.o->type == ObjType::STRING) {
			return compareStrings(static_cast<ObjString*>(a.o), static_cast<ObjString*>(b.o));
		}
		luaError("attempt to compare wrong datatypes");
	}
} // namespace tinylc
