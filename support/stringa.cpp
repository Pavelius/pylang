#include "stringa.h"
#include "stringbuilder.h"

unsigned stringa::find(const char* v, size_t len) const {
	if(v && len) {
		auto pe = end();
		auto s = *v;
		for(auto p = begin(); p < pe; p++) {
			if(*p != s)
				continue;
			unsigned n1 = pe - p - 1;
			if(n1 < len)
				return -1;
			if(memcmp(p + 1, v + 1, len) == 0)
				return p - begin();
		}
	}
	return -1;
}

unsigned stringa::find(const char* v) const {
	return find(v, zlen(v));
}

unsigned stringa::add(const char* v, size_t len) {
	auto result = count;
	reserve(result + len + 1);
	memcpy(ptr(result), v, len + 1);
	setcount(result + len + 1);
	return result;
}

unsigned stringa::add(const char* v) {
	if(!v || v[0] == 0)
		return 0;
	auto c = zlen(v);
	auto i = find(v, c);
	if(i != 0xFFFFFFFF)
		return i;
	return add(v, c);
}