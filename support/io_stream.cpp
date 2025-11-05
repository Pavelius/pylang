#include "io_stream.h"
#include "stringbuilder.h"
#include "slice.h"

io::stream& io::stream::operator<<(const int n) {
	char temp[32]; stringbuilder sb(temp);
	sb.add("%1i", n);
	return *this << temp;
}

io::stream&	io::stream::operator<<(const char* t) {
	if(!t)
		return *this;
	while(*t) {
		char temp[8];
		char* s1 = temp;
		unsigned u = szget(&t, codepage::W1251);
		s1 = szput(s1, u, codepage::UTF8);
		write(temp, s1 - temp);
	}
	return *this;
}

unsigned char io::stream::get() {
	unsigned char r = 0;
	read(&r, 1);
	return r;
}

unsigned short io::stream::getLE16() {
	unsigned char u2 = get();
	unsigned char u1 = get();
	return (u2 << 8) | u1;
}

unsigned io::stream::getLE32() {
	unsigned char u4 = get();
	unsigned char u3 = get();
	unsigned char u2 = get();
	unsigned char u1 = get();
	return (u4 << 24) | (u3 << 16) | (u2 << 8) | u1;
}

unsigned io::stream::get32() {
	unsigned v; read(&v, sizeof(v));
	return v;
}

short unsigned io::stream::get16() {
	short unsigned v; read(&v, sizeof(v));
	return v;
}

void* loadb(const char* url, int* size, int additional) {
	void* p = 0;
	if(size)
		*size = 0;
	if(!url || url[0] == 0)
		return 0;
	io::file file(url, StreamRead);
	if(!file)
		return 0;
	int s = file.seek(0, SeekEnd) + additional;
	file.seek(0, SeekSet);
	p = new char[s];
	memset(p, 0, s);
	file.read(p, s);
	if(size)
		*size = s;
	return p;
}

char* loadt(const char* url, int* size) {
	int s1;
	if(size)
		*size = 0;
	unsigned char* p = (unsigned char*)loadb(url, &s1, 1);
	if(!p)
		return 0;
	if(p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
		// UTF8
		// Перекодируем блок через декодировщик. Может быть только меньше,
		// так как системная кодировка ANSI
		szencode((char*)p, s1, codepage::W1251, (char*)p + 3, s1, codepage::UTF8);
	}
	if(size)
		*size = s1;
	return (char*)p;
}

const char* szext(const char* path) {
	for(const char* r = zend((char*)path); r > path; r--) {
		if(*r == '.')
			return r + 1;
		else if(*r == '\\' || *r == '/')
			return 0;
	}
	return 0;
}

const char* szfname(const char* path) {
	for(const char* r = zend((char*)path); r > path; r--) {
		if(*r == '\\' || *r == '/')
			return r + 1;
	}
	return path;
}

char* szfnamewe(char* result, const char* name) {
	stringbuilder sb(result, result + 260); sb.clear();
	sb.addv(szfname(name), 0);
	char* p = (char*)szext(result);
	if(p && p != result)
		p[-1] = 0;
	return result;
}

char* szurl(char* result, const char* base_url, const char* folder, const char* name, const char* ext, const char* suffix) {
	stringbuilder sb(result, result + 260); sb.clear();
	if(base_url)
		sb.add(base_url);
	if(folder) {
		if(sb)
			sb.add("/");
		sb.add(folder);
	}
	if(name) {
		if(sb)
			sb.add("/");
		sb.add(name);
	}
	if(suffix)
		sb.add(suffix);
	if(ext) {
		if(sb)
			sb.add(".");
		sb.add(ext);
	}
	return result;
}

unsigned szget(const char** input, codepage code) {
	const unsigned char* p;
	unsigned result;
	switch(code) {
	case codepage::UTF8:
		p = (unsigned char*)*input;
		result = *p++;
		if(result >= 192 && result <= 223)
			result = (result - 192) * 64 + (*p++ - 128);
		else if(result >= 224 && result <= 239) {
			result = (result - 224) * 4096 + (p[0] - 128) * 64 + (p[1] - 128);
			p += 2;
		}
		*input = (const char*)p;
		return result;
	case codepage::U16LE:
		p = (unsigned char*)*input;
		result = p[0] | (p[1] << 8);
		*input = (const char*)(p + 2);
		return result;
	case codepage::W1251:
		result = (unsigned char)*(*input)++;
		if(((unsigned char)result >= 0xC0))
			return result - 0xC0 + 0x410;
		else switch(result) {
		case 0xB2: return 0x406;
		case 0xAF: return 0x407;
		case 0xB3: return 0x456;
		case 0xBF: return 0x457;
		}
		return result;
	default:
		return *(*input)++;
	}
}

char* szput(char* p, unsigned value, codepage code) {
	switch(code) {
	case codepage::UTF8:
		if(((unsigned short)value) < 128)
			*p++ = (unsigned char)value;
		else if(((unsigned short)value) < 2047) {
			*p++ = (unsigned char)(192 + (((unsigned short)value) / 64));
			*p++ = (unsigned char)(128 + (((unsigned short)value) % 64));
		} else {
			*p++ = (unsigned char)(224 + (((unsigned short)value) / 4096));
			*p++ = (unsigned char)(128 + ((((unsigned short)value) / 64) % 64));
			*p++ = (unsigned char)(224 + (((unsigned short)value) % 64));
		}
		break;
	case codepage::W1251:
		if(value >= 0x410 && value <= 0x44F)
			value = value - 0x410 + 0xC0;
		else switch(value) {
		case 0x406: value = 0xB2; break; // I
		case 0x407: value = 0xAF; break; // Ї
		case 0x456: value = 0xB3; break;
		case 0x457: value = 0xBF; break;
		}
		*p++ = (unsigned char)value;
		break;
	case codepage::U16LE:
		*p++ = (unsigned char)(value & 0xFF);
		*p++ = (unsigned char)(((unsigned)value >> 8));
		break;
	case codepage::U16BE:
		*p++ = (unsigned char)(((unsigned)value >> 8));
		*p++ = (unsigned char)(value & 0xFF);
		break;
	default:
		*p++ = (unsigned char)value;
		break;
	}
	return p;
}

void szencode(char* output, int output_count, codepage output_code, const char* input, int input_count, codepage input_code) {
	char* s1 = output;
	char* s2 = s1 + output_count;
	const char* p1 = input;
	const char* p2 = p1 + input_count;
	while(p1 < p2 && s1 < s2)
		s1 = szput(s1, szget(&p1, input_code), output_code);
	if(s1 < s2) {
		s1[0] = 0;
		if((output_code == codepage::U16BE || output_code == codepage::U16LE) && (s1 + 1) < s2)
			s1[1] = 0;
	}
}
