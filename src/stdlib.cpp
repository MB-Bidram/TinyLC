#include "stdlib.hpp"

namespace tinylc {
	inline const char* objTypeName(ObjType t) {
		switch (t) {
			case ObjType::STRING: { return "string"; };
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
			};
	
			default: {
				std::cout
					<< objTypeName(obj->type)
					<< ": "
					<< "0x" << std::hex << (uintptr_t)obj << std::dec;
				break;
			};
		}
	}



	inline bool stringToNumber(const ObjString* s, double& out) {
		char* end;
		out = std::strtod(s->chars, &end);
		if (end == s->chars) { return false; }
		while (*end == ' ' || *end == '\t' || *end == '\n') { end++; }
		return *end == '\0';
	}


	inline bool toNumber(const Value& v, double& out) {
		switch (v.type) {
			case Type::INT: {
				out = (double)v.i;
				return true;
			};
	
			case Type::NUM: {
				out = v.n;
				return true;
			};
	
			case Type::OBJ: {
				if (v.o->type == ObjType::STRING) {
					return stringToNumber(
						static_cast<ObjString*>(v.o),
						out
					);
				}
				return false;
			};
	
			default: { return false; };
		}
	}

	void _printValue(const Value& v) {
		switch (v.type) {
		case Type::INT: {
			std::cout << v.i;
			break;
		};
		case Type::NUM: {
			std::cout << v.n;
			break;
		};
		case Type::BOOL: {
			std::cout << (v.b ? "true" : "false");
			break;
		};
		case Type::NIL: {
			std::cout << "nil";
			break;
		};
		case Type::OBJ: {
			_printObject(v.o);
			break;
		};

		default: {
			printf("Unhandled Value/Type (Type::*) (Int)%d\n", v.type);
			std::abort();
		};
		}
	}

	const char* typeName(Type t) {
		switch (t) {
		case Type::NIL: {
			return "nil";
		};
		case Type::INT: {
			return "number";
		};
		case Type::NUM: {
			return "number";
		};
		case Type::BOOL: {
			return "boolean";
		};
		case Type::OBJ: {
			return "object";
		};
		}
		return "unknown";
	}

	double gc(State& S, GCOp op) {
		switch (op) {
		case GCOp::STOP: {
			S.gcEnabled = false;
			return 0;
		};
		case GCOp::RESTART: {
			S.gcEnabled = true;
			return 0;
		};
		case GCOp::COLLECT: {
			if (S.gcEnabled) {
				collectGarbage(S);
			};
			return 0;
		};
		case GCOp::COUNT: {
			return (double)S.bytesAllocated / 1024.0;
		};
		case GCOp::STEP: {
			if (S.gcEnabled) {
				collectGarbage(S);
			};
			return 0;
		};
		}

		return 0;
	}

	Value toString(const Value& v) {
		State& S = runtime();

		switch (v.type) {
		case Type::NIL: {
			return makeString("nil");
		};
		case Type::BOOL: {
			return makeString(v.b ? "true" : "false");
		};

		case Type::INT:
		case Type::NUM: {
			double n = (v.type == Type::INT) ? (double)v.i : v.n;
			// cached
			for (uint32_t i = 0; i < S.numStrCount; ++i) {
				if (S.numStrCache[i].num == n) {
					return Value((Obj*)S.numStrCache[i].str);
				};
			}

			char buf[64];
			if (v.type == Type::INT) {
				std::snprintf(buf, sizeof(buf), "%lld", (long long)v.i);
			} else {
				std::snprintf(buf, sizeof(buf), "%.14g", n);
			}

			ObjString* s = newString(buf, std::strlen(buf));

			if (S.numStrCount < S.numStrCap) {
				S.numStrCache[S.numStrCount++] = {n, s};
			} else if (S.numStrCap > 0) {
				uint32_t slot = ((uint32_t)n) & (S.numStrCap - 1);
				S.numStrCache[slot] = {n, s};
			}

			return Value((Obj*)s);
		};

		case Type::OBJ: {
			if (v.o->type == ObjType::STRING) {
				return v;
			} // tostring(type: string) -> string (itself)

			char buf[128];
			switch (v.o->type) {
			case ObjType::TABLE: {
				std::snprintf(buf, sizeof(buf), "table: %p", (void*)v.o);
				return makeString(buf);
			};
			default: {
				std::snprintf(buf, sizeof(buf), "object: %p", (void*)v.o);
				return makeString(buf);
			};
			}
		};
		}

		return makeString("unknown");
	}

	Value tonumber(const Value& v) {
		double out;
		if (toNumber(v, out)) {
			if ((int64_t)out == out) {
				return Value((int64_t)out);
			}
			return Value(out);
		}
		return Value(); // nil
	}

	Value makeTable() {
		return Value((Obj*)newTable());
	}
} // namespace tinylc