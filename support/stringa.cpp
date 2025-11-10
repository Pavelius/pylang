#include "stringa.h"
#include "stringbuilder.h"

int find_string(const char* pb, const char* pe, const char* value, size_t len) {
	if(value && len) {
		auto s = *value;
		for(auto p = pb; p < pe; p++) {
			if(*p != s)
				continue;
			unsigned n1 = pe - p - 1;
			if(n1 < len)
				return -1;
			if(memcmp(p + 1, value + 1, len) == 0)
				return p - pb;
		}
	}
	return -1;
}

int add_string(array& source, const char* value, size_t len) {
	auto result = source.count;
	source.reserve(result + len + 1);
	memcpy(source.ptr(result), value, len + 1);
	source.setcount(result + len + 1);
	return result;
}

int stringa::find(const char* v) const {
	return find_string(begin(), end(), v, zlen(v));
}

int stringa::add(const char* v) {
	if(!v || v[0] == 0)
		return 0;
	auto c = zlen(v);
	auto i = find_string(begin(), end(), v, c);
	if(i == -1)
		return add_string(*this, v, c);
	return i;
}