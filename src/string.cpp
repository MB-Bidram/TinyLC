#include "string.hpp"

namespace tinylc {
	void growInternTable(State& S) {
		size_t newCap = S.capacity * 2;

		InternNode** newBuckets = (InternNode**)xmalloc(sizeof(InternNode*) * newCap);

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

	ObjString* newString(const char* chars, size_t length) {
		State& S = runtime();

		uint32_t hash = hashString(chars, length);
		size_t index = hash % S.capacity;

		for (InternNode* it = S.buckets[index]; it; it = it->next) {
			ObjString* s = it->str;
			if (s->hash == hash && s->length == length &&
				std::memcmp(s->chars, chars, length) == 0) {
				return s;
			}
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
		if (S.count > S.capacity * 0.75) {
			growInternTable(S);
		};

		return s;
	}

	Value makeString(const char* s) {
		return Value(newString(s, std::strlen(s)));
	}

	int compareStrings(const ObjString* a, const ObjString* b) {
		size_t minLen = a->length < b->length ? a->length : b->length;
		int cmp = std::memcmp(a->chars, b->chars, minLen);
		if (cmp != 0)
			return cmp;
		if (a->length == b->length)
			return 0;
		return (a->length < b->length) ? -1 : 1;
	}
} // namespace tinylc