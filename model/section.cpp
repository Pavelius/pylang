#include "slice.h"
#include "section.h"
#include "stringa.h"

sectioni sections[LastSection + 1];

void set_value(sectionn sec, unsigned offset, unsigned size, int value) {
	if(sec == -1)
		return;
	auto& e = sections[sec];
	if(!e.data)
		return;
	auto p = (unsigned char*)e.data + offset;
	switch(size) {
	case 1: *((char*)p) = (char)value; break;
	case 2: *((short*)p) = (short)value; break;
	case 4: *((int*)p) = value; break;
	default: break;
	}
}

int get_value(sectionn sec, unsigned offset, unsigned size) {
	if(sec == -1)
		return 0;
	auto& e = sections[sec];
	if(!e.data)
		return 0;
	auto p = (unsigned char*)e.data + offset;
	switch(size) {
	case 1: return *((char*)p);
	case 2: return *((short*)p);
	case 4: return *((int*)p);
	default: return 0;
	}
}

void sectioni::reserve(unsigned new_size) {
	if(!isvirtual())
		data = realloc_data(data, new_size, size_maximum);
}

void sectioni::add(const void* source, size_t source_size) {
	reserve(size + source_size);
	if(data && source)
		memcpy((unsigned char*)data + size, source, source_size);
	size += source_size;
}

char* sectioni::ptr(unsigned offset) const {
	if(data)
		return (char*)data + offset;
	return 0;
}

unsigned sectioni::addstring(const char* value, size_t len) {
	unsigned i = -1;
	if(isvirtual())
		return i;
	if(data)
		i = find_string((char*)data, (char*)data + size, value, len);
	if(i == -1) {
		i = size;
		add(value, len + 1);
	}
	return i;
}

void initialize_sections() {
	sections[LocalSection].setvirtual();
	sections[UDataSection].setvirtual();
}