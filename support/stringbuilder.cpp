/////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Pavel Chistyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stringbuilder.h"
#include "slice.h"

char current_locale[4] = {"ru"};
static const char spaces[] = " \n\t\r.,!?;:";

stringbuilder::fncustom stringbuilder::custom = default_string;

struct stringbuilder::grammar {
	const char* name;
	const char* change;
	size_t name_size;
	size_t change_size;
	constexpr grammar() : name(0), change(0), name_size(0), change_size(0) {}
	constexpr grammar(const char* name, const char* change) :
		name(name), change(change), name_size(zlen(name)), change_size(zlen(change)) {
	}
	operator bool() const { return name != 0; }
};

namespace {
struct genderi {
	const char* name;
	int value;
	unsigned name_size;
	constexpr genderi() : name(0), value(0), name_size(0) {}
	constexpr genderi(const char* name, int value) : name(name), value(value), name_size(zlen(name)) {}
	operator bool() const { return name != 0; }
};
}

static const char* psnum16(const char* p, long& value) {
	int result = 0;
	const int radix = 16;
	while(*p) {
		char a = *p;
		if(a >= '0' && a <= '9') {
			result = result * radix;
			result += a - '0';
		} else if(a >= 'a' && a <= 'f') {
			result = result * radix;
			result += a - 'a' + 10;
		} else if(a >= 'A' && a <= 'F') {
			result = result * radix;
			result += a - 'A' + 10;
		} else
			break;
		p++;
	}
	value = result;
	return p;
}

static const char* psnum10(const char* p, long& value) {
	int result = 0;
	const int radix = 10;
	while(*p) {
		char a = *p;
		if(a >= '0' && a <= '9') {
			result = result * radix;
			result += a - '0';
		} else
			break;
		p++;
	}
	value = result;
	return p;
}

bool equal(const char* p, const char* s) {
	if(!p || !s)
		return false;
	while(*s && *p)
		if(*p++ != *s++)
			return false;
	return *s == *p;
}

const char* psnum(const char* p, long& value) {
	value = 0;
	if(!p)
		return 0;
	bool sign = false;
	// Sign setup
	if(*p == '-') {
		sign = true;
		p++;
	}
	// Choose digit radix
	if(p[0] == '0' && p[1] == 'x') {
		p += 2;
		p = psnum16(p, value);
	} else
		p = psnum10(p, value);
	if(sign)
		value = -value;
	return p;
}

const char* psnum(const char* p, int& value) {
	long temp;
	auto result = psnum(p, temp);
	value = temp;
	return result;
}

const char* psnum(const char* p, short& value) {
	long temp;
	auto result = psnum(p, temp);
	value = (short)temp;
	return result;
}

const char* psbon(const char* p, int& bonus) {
	if(*p == '-')
		p = psnum(p, bonus);
	else if(*p == '+')
		p = psnum(p + 1, bonus);
	else
		bonus = 0;
	return p;
}

const char*	psidf(const char* p, stringbuilder& result) {
	result.clear();
	return result.psidf(p);
}

static const char* word_end(const char* ps) {
	while(*ps) {
		for(auto e : spaces) {
			if(*ps == e)
				return ps;
		}
		ps++;
	};
	return ps;
}

static const char* skip_space(const char* ps) {
	while(*ps) {
		for(auto e : spaces) {
			if(*ps != e)
				return ps;
		}
		ps++;
	};
	return ps;
}

static bool is_space(char sym) {
	for(auto e : spaces) {
		if(sym == e)
			return true;
	}
	return false;
}

const char* skipcr(const char* p) {
	if(*p == '\n') {
		p++;
		if(*p == '\r')
			p++;
	} else if(*p == '\r') {
		p++;
		if(*p == '\n')
			p++;
	}
	return p;
}

const char* skipsp(const char* p) {
	while(*p == ' ' || *p == '\t')
		p++;
	return p;
}

const char* skipspcr(const char* p) {
	while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	return p;
}

const char* skipline(const char* p) {
	while(*p && !(*p == 10 || *p == 13))
		p++;
	return p;
}

int get_line_number(const char* start, const char* position) {
	auto p = start;
	auto r = 0;
	while(*p && p < position) {
		p = skipline(p);
		p = skipcr(p);
		r++;
	}
	return r;
}

bool szstart(const char* text, const char* name) {
	while(*name) {
		if(*name++ != *text++)
			return false;
	}
	return true;
}

bool szmatch(const char* text, const char* name) {
	while(*name) {
		if(*name++ != *text++)
			return false;
	}
	if(ischa(*text))
		return false;
	return true;
}

bool szpmatch(const char* string, const char* pattern) {
	auto p = string;
	while(*p) {
		auto symbol = *pattern++;
		switch(symbol) {
		case 0:
			return false;
		case '*':
			while(*p && *p != *pattern)
				p++;
			continue;
		case '?':
			p++;
			continue;
		}
		if(*p != symbol) {
			while(symbol && symbol != ',')
				symbol = *pattern++;
			if(symbol == ',') {
				p = string;
				pattern = skipsp(pattern);
				continue;
			}
			return false;
		}
		p++;
	}
	return (*pattern == 0) || (*pattern == ',');
}

unsigned char upper_symbol(unsigned char u) {
	if(u >= 0x61 && u <= 0x7A)
		return u - 0x61 + 0x41;
	else if(u >= 0xE0)
		return u - 0xE0 + 0xC0;
	return u;
}

unsigned char lower_symbol(unsigned char u) {
	if(u >= 0x41 && u <= 0x5A)
		return u - 0x41 + 0x61;
	else if(u >= 0xC0 && u <= 0xDF)
		return u - 0xC0 + 0xE0;
	return u;
}

int szcmpi(const char* p1, const char* p2) {
	while(true) {
		auto s1 = upper_symbol(*p1++);
		auto s2 = upper_symbol(*p2++);
		if(!s1 || !s2 || s1 != s2)
			return s1 - s2;
	}
}

int szcmp(const char* p1, const char* p2) {
	while(true) {
		auto s1 = *p1++;
		auto s2 = *p2++;
		if(!s1 || !s2 || s1 != s2)
			return s1 - s2;
	}
}

const char* str_count(const char* id, int count) {
	static char temp[64]; stringbuilder sb(temp);
	switch(count) {
	case 0: case 1: return id;
	case 2: case 3: case 4:
		sb.addv(id, 0);
		sb.addv("Of", 0);
		break;
	default:
		sb.addv(id, 0);
		sb.addv("Pl", 0);
		break;
	}
	return temp;
}

void default_string(stringbuilder& sb, const char* id) {
	// sb.addv(getnm(id), 0);
}

void stringbuilder::lower() {
	for(auto p = pb; *p; p++)
		*p = lower_symbol(*p);
}

void stringbuilder::upper() {
	for(auto p = pb; *p; p++)
		*p = upper_symbol(*p);
}

const char* stringbuilder::readvariable(const char* p) {
	char temp[260]; stringbuilder s1(temp);
	p = s1.psidf(p);
	if(custom)
		custom(*this, temp);
	return p;
}

void stringbuilder::adduint(unsigned value, int precision, const int radix) {
	char temp[32]; int i = 0;
	if(!value)
		temp[i++] = 0;
	else {
		while(value) {
			temp[i++] = (value % radix);
			value /= radix;
		}
	}
	while(precision-- > i)
		add("0");
	while(i) {
		auto v = temp[--i];
		if(p < pe) {
			if(v < 10)
				*p++ = '0' + v;
			else
				*p++ = 'A' + (v - 10);
		}
	}
	p[0] = 0;
}

void stringbuilder::addint(int value, int precision, const int radix) {
	if(value < 0) {
		add("-");
		value = -value;
	}
	adduint(value, precision, radix);
}

void stringbuilder::addnz(const char* format, unsigned count) {
	unsigned n = pe - p - 1;
	if(count > n)
		count = n;
	memcpy(p, format, count); p[count] = 0;
	p += count;
}

const char* stringbuilder::readformat(const char* src, const char* vl) {
	if(*src == ',' || *src == '.' || *src == ' ' || *src == 10 || *src == 13 || *src == 9) {
		if(p < pe)
			*p++ = '%';
		*p = 0;
		return src;
	} else if(*src == '%') {
		src++;
		if(p < pe)
			*p++ = '%';
		*p = 0;
		return src;
	}
	*p = 0;
	char prefix = 0;
	char padeg = 0;
	if(*src == '+' || *src == '-')
		prefix = *src++;
	if(*src == '$' || *src == '@' || *src == '~' || *src == '*')
		padeg = *src++;
	auto p0 = p;
	if(*src >= '0' && *src <= '9') {
		long pn = 0, pnp = 0;
		if(isnum(*src))
			src = psnum10(src, pn);
		if(src[0] == '.' && isnum(src[1]))
			src = psnum10(src + 1, pnp);
		if(*src == 'i') {
			src++;
			auto value = ((long*)vl)[pn - 1];
			if(prefix == '+' && value >= 0) {
				if(p < pe)
					*p++ = '+';
			}
			addint(value, pnp, 10);
		} else if(*src == 'h') {
			src++;
			adduint((unsigned)(((long*)vl)[pn - 1]), pnp, 16);
		} else if(*src == 'c') {
			src++;
			if(p < pe)
				*p++ = (((char*)vl)[pn - 1]);
			if(p < pe)
				*p = 0;
		} else {
			if(((char**)vl)[pn - 1]) {
				auto p1 = ((char**)vl)[pn - 1];
				switch(padeg) {
				case '$': addof(p1); break;
				case '@': addto(p1); break;
				case '~': addby(p1); break;
				default:
					while(*p1 && p < pe)
						*p++ = *p1++;
					if(p < pe)
						*p = 0;
					break;
				}
			}
		}
	} else {
		char temp[260]; stringbuilder sb(temp);
		src = readvariable(src);
		switch(padeg) {
		case '$': sb.copy(p0); p = p0; addof(temp); break;
		case '@': sb.copy(p0); p = p0; addto(temp); break;
		case '~': sb.copy(p0); p = p0; addby(temp); break;
		default: break;
		}
	}
	if(p0 && p0 != p) {
		switch(prefix) {
		case '-': *p0 = lower_symbol(*p0); break;
		case '+': *p0 = upper_symbol(*p0); break;
		default: break;
		}
	}
	return src;
}

void stringbuilder::add(char sym) {
	if(p < pe) {
		*p++ = sym;
		*p = 0;
	}
}

void stringbuilder::add(const char* format, ...) {
	XVA_FORMAT(format);
	addv(format, format_param);
}

void stringbuilder::addv(const char* src, const char* vl) {
	if(!p)
		return;
	p[0] = 0;
	if(!src)
		return;
	while(true) {
		switch(*src) {
		case 0: *p = 0; return;
		case '%': src = readformat(src + 1, vl); break;
		default:
			if(p < pe)
				*p++ = *src;
			src++;
			break;
		}
	}
}

void stringbuilder::addsep(char separator) {
	if(p <= pb || p >= pe)
		return;
	if(p[-1] == separator)
		return;
	switch(separator) {
	case ' ':
		if(p[-1] == '\n' || p[-1] == '\t' || p[-1] == '[' || p[-1] == '(')
			return;
		break;
	case '.':
		if(p[-1] == '?' || p[-1] == '!' || p[-1] == ':')
			return;
		break;
	}
	*p++ = separator;
}

void stringbuilder::addx(char separator, const char* format, const char* format_param) {
	if(!format || format[0] == 0)
		return;
	addsep(separator);
	addv(format, format_param);
}

void stringbuilder::addx(const char* separator, const char* format, const char* format_param) {
	if(!format || format[0] == 0)
		return;
	if(p != pb)
		add(separator);
	addv(format, format_param);
}

void stringbuilder::change(char s1, char s2) {
	for(auto p = pb; p < pe; p++) {
		if(*p == 0)
			break;
		if(*p == s1)
			*p++ = s2;
	}
}

void stringbuilder::copy(const char* v) {
	while(p < pe && *v)
		*p++ = *v++;
	p[0] = 0;
}

const char*	szfind(const char* source, const char* value) {
	if(!value || !source || !value[0])
		return 0;
	auto s = value[0];
	for(auto p = source; *p; p++) {
		if(*p == s && szstart(p, value))
			return p;
	}
	return 0;
}

const char*	szfindend(const char* source, const char* value) {
	if(!value || !source || !value[0])
		return 0;
	auto s = value[0];
	for(auto p = source; *p; p++) {
		if(*p == s && equal(p, value))
			return p;
	}
	return 0;
}

void stringbuilder::change(const char* s1, const char* s2) {
	if(!s1 || s1[0] == 0)
		return;
	auto n1 = zlen(s1);
	auto n2 = zlen(s2);
	for(auto p = pb; p < pe; p++) {
		if(*p == 0)
			break;
		if(*p == s1[0]) {
			size_t n = this->pe - p;
			if(n1 > n)
				return;
			if(memcmp(p, s1, n1) != 0)
				continue;
			auto c = n2;
			if(c > size_t(pe - this->p - 1))
				c = pe - this->p - 1;
			if(c > n1)
				memmove(p + c, p + n1, pe - p - c);
			memcpy(p, s2, n2);
			p += n2 - 1;
		}
	}
}

void stringbuilder::addgr(const char* s, const grammar* source, const char* def) {
	auto ps = skip_space(s);
	while(*ps) {
		auto pw = word_end(ps);
		unsigned s1 = pw - ps;
		auto found = false;
		for(auto pg = source; *pg; pg++) {
			auto s2 = pg->name_size;
			if(pg->name_size > s1)
				continue;
			if(memcmp(pw - s2, pg->name, s2) == 0) {
				auto s3 = pg->change_size;
				memcpy(p, ps, s1 - s2);
				memcpy(p + (s1 - s2), pg->change, s3);
				p += (s1 - s2 + s3);
				if(pw[0] == 0)
					def = 0;
				found = true;
				break;
			}
		}
		if(!found) {
			memcpy(p, ps, pw - ps);
			p += pw - ps;
		}
		ps = pw;
		while(*ps) {
			if(is_space(*ps))
				*p++ = *ps++;
			break;
		}
	}
	p[0] = 0;
	if(def)
		addv(def, 0);
}

int get_number(const char* value) {
	if(equal(value, "true"))
		return 1;
	if(equal(value, "false"))
		return 0;
	if(equal(value, "null"))
		return 0;
	long int_value = 0;
	psnum(value, int_value);
	return int_value;
}

const char* stringbuilder::psidf(const char* pb) {
	if(*pb == '(') {
		pb++;
		while(*pb && *pb != ')') {
			if(p < pe)
				*p++ = *pb;
			pb++;
		}
		if(*pb == ')')
			pb++;
	} else {
		while(*pb && (ischa(*pb) || isnum(*pb) || *pb == '_')) {
			if(p < pe)
				*p++ = *pb++;
			else
				break;
		}
	}
	*p = 0;
	return pb;
}

void stringbuilder::addch(char sym) {
	switch(sym) {
	case -85: case -69: add('\"'); break;
	case -72: add('е'); break;
	case -105: case 17: case 0x14: add('-'); break;
	case -123: add("..."); break;
	case 0x19: add("'"); break;
	default: add(sym);
	}
}

const char* stringbuilder::psstrlf(const char* p) {
	while(*p) {
		if(p[0] == '\\' && p[1] == 'n') {
			add('\n');
			p += 2;
			continue;
		} else if(*p == '\n' || *p == '\r')
			break;
		addch(*p++);
	}
	return p;
}

const char* stringbuilder::psparam(const char* p) {
	while(*p) {
		if(*p == '\n' || *p == '\r' || *p == ',')
			break;
		addch(*p++);
	}
	return p;
}

const char* stringbuilder::psstr(const char* pb, char end_symbol) {
	p[0] = 0;
	if(!pb)
		return 0;
	while(*pb) {
		if(*pb == end_symbol) {
			*p++ = 0;
			return pb + 1;
		} else if(*pb != '\\') {
			if(p < pe)
				*p++ = *pb;
			pb++;
			continue;
		}
		pb++;
		switch(*pb) {
		case 'n':
			if(p < pe)
				*p++ = '\n';
			pb++;
			break;
		case 'r':
			if(p < pe)
				*p++ = '\r';
			pb++;
			break;
		case 't':
			if(p < pe)
				*p++ = '\t';
			pb++;
			break;
		case 'b':
			if(p < pe)
				*p++ = '\b';
			pb++;
			break;
		case 'f':
			if(p < pe)
				*p++ = '\f';
			pb++;
			break;
		case 'v':
			if(p < pe)
				*p++ = '\v';
			pb++;
			break;
		case '\n': case '\r':
			while(*pb == '\n' || *p == '\r')
				pb = skipcr(pb);
			break;
		default:
			if(p < pe)
				*p++ = *pb;
			pb++;
			break;
		}
	}
	return pb;
}

void stringbuilder::addof(const char* s) {
	static grammar map[] = {
		{"ая", "ой"},
		{"би", "би"},
		{"ел", "ла"},
		{"ие", "ия"},
		{"ий", "ого"},
		{"ка", "ки"},
		{"ль", "ля"},
		{"ны", "н"},
		{"ое", "ого"},
		{"рь", "ря"},
		{"сы", "сов"},
		{"ый", "ого"},
		{"а", "ы"},
		{"и", "ей"},
		{"о", "а"},
		{"ы", ""},
		{"ь", "и"},
		{"я", "и"},
		{}};
	addgr(s, map, "а");
}

void stringbuilder::addnounf(const char* s) {
	static grammar map[] = {
		{"ий", "ая"},
		{"ой", "ая"},
		{"ый", "ая"},
		{}};
	addgr(s, map, "");
}

void stringbuilder::addnouni(const char* s) {
	static grammar map[] = {
		{"ний", "нее"},
		{"ий", "ое"},
		{"ой", "ое"},
		{"ый", "ое"},
		{}};
	addgr(s, map, "");
}

void stringbuilder::addnounx(const char* s) {
	static grammar map[] = {
		{"ий", "ие"},
		{"ый", "ые"},
		{}};
	addgr(s, map, "");
}

void stringbuilder::addby(const char* s) {
	static grammar map[] = {
		{"бой", "боем"},
		{"ая", "ой"},
		{"ый", "ым"},
		{"ое", "ым"},
		{"ой", "ым"},
		{"би", "би"},
		{"ий", "им"},
		{"ец", "цем"},
		{"ки", "ками"},
		{"ти", "ями"},
		{"й", "ем"}, {"ь", "ем"}, {"е", "ем"},
		{"и", "ами"},
		{"ы", "ами"},
		{"а", "ой"},
		{"ч", "чем"},
		{}};
	addgr(s, map, "ом");
}

void stringbuilder::addto(const char* s) {
	static grammar map[] = {
		{"а", "е"},
		{"о", "у"},
		{"ы", "ам"},
		{}
	};
	addgr(s, map, "у");
}

int gender_by_name(const char* s) {
	static genderi source[] = {
		{"а", 1},
		{"о", 2},
		{"ы", 3},
		{"я", 1},
		{"ье", 2},
	};
	auto ps = skip_space(s);
	auto pw = word_end(ps);
	unsigned s1 = pw - ps;
	for(auto& e : source) {
		auto s2 = e.name_size;
		if(e.name_size > s1)
			continue;
		if(memcmp(pw - s2, e.name, s2) == 0)
			return e.value;
	}
	return 0;
}

void stringbuilder::adjective(const char* name, int m) {
	switch(m) {
	case 1: addnounf(name); break;
	case 2: addnouni(name); break;
	case 3: addnounx(name); break;
	default: add(name); break;
	}
}

void stringbuilder::trimr() {
	while(p > pb && (p[-1] == ' ' || p[-1] == '\t' || p[-1] == '\n')) {
		p[-1] = 0;
		p--;
	}
}

const char* str(const char* format, ...) {
	static char temp[1024]; stringbuilder sb(temp);
	XVA_FORMAT(format);
	sb.addv(format, format_param);
	return temp;
}

const char* ids(const char* p1, const char* p2) {
	static char temp[64]; stringbuilder sb(temp);
	sb.addv(p1, 0);
	sb.addv(p2, 0);
	return temp;
}

const char* ids(const char* p1, const char* p2, const char* p3) {
	static char temp[64]; stringbuilder sb(temp);
	sb.addv(p1, 0);
	sb.addv(p2, 0);
	sb.addv(p3, 0);
	return temp;
}
