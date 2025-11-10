#include "bsdata.h"
#include "slice.h"
#include "section.h"
#include "stringa.h"

BSDATAC(sectioni, 256)

extern "C" void exit(int exit_code) noexcept;

extern "C" void* malloc(long unsigned size);
extern "C" void* realloc(void *ptr, long unsigned size);
extern "C" void	free(void* pointer);

static unsigned rmoptimal(unsigned need_count) {
	const unsigned mc = 4 * 256 * 256;
	unsigned m = 8;
	while(m < mc) {
		m = m << 1;
		if(need_count < m)
			return m;
	}
	return need_count + mc;
}

void sectioni::add(const void* source, size_t size) {
	reserve(size);
	if(data)
		memcpy((unsigned char*)data, source, size);
	this->size += size;
}

void set_value(int sec, int offset, int size, int value) {
	if(sec == -1)
		return;
	auto& e = bsdata<sectioni>::get(sec);
	if(!e.data)
		return;
	auto p = (char*)e.data + offset;
	switch(size) {
	case 1: *((char*)p) = (char)value; break;
	case 2: *((short*)p) = (short)value; break;
	case 4: *((int*)p) = value; break;
	default: break;
	}
}

int get_value(int sec, int offset, int size) {
	if(sec == -1)
		return 0;
	auto& e = bsdata<sectioni>::get(sec);
	if(!e.data)
		return 0;
	auto p = (char*)e.data + offset;
	switch(size) {
	case 1: return *((char*)p);
	case 2: return *((short*)p);
	case 4: return *((int*)p);
	default: return 0;
	}
}

void sectioni::reserve(size_t size) {
	auto total = this->size + size;
	if(!isvirtual())
		data = realloc_data(data, total, size_maximum);
}

char* sectioni::ptr(int offset) const {
	if(data)
		return (char*)data + offset;
	return 0;
}

sectioni& section(int sec) {
	return bsdata<sectioni>::elements[sec];
}

int sectioni::addstring(const char* value, size_t len) {
	auto i = -1;
	if(isvirtual())
		return i;
	if(data)
		i = find_string((char*)data, (char*)data + size, value, len);
	if(i==-1) {
		i = size;
		reserve(size + len + 1);
		memcpy((char*)data + i, value, len + 1);
		size = i + len + 1;
	}
	return i;
}

void initialize_sections() {
	section(LocalSection).setvirtual();
	section(UDataSection).setvirtual();
}