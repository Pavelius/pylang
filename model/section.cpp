#include "bsdata.h"
#include "slice.h"
#include "section.h"

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
	if(isvirtual())
		return;
	if(total <= size_maximum)
		return;
	size_maximum = rmoptimal(total);
	if(data) {
		auto p = realloc(data, size_maximum);
		if(!p)
			exit(0);
		data = p;
	} else
		data = malloc(size_maximum);
}

char* sectioni::ptr(int offset) const {
	if(data)
		return (char*)data + offset;
	return 0;
}

sectioni& section(int sec) {
	return bsdata<sectioni>::elements[sec];
}

void initialize_sections() {
	section(LocalSection).setvirtual();
	section(UDataSection).setvirtual();
}