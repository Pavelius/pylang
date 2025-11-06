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

sectioni& section(sectionn v) {
	return bsdata<sectioni>::elements[v];
}

void initialize_sections() {
	section(LocalSection).setvirtual();
	section(UDataSection).setvirtual();
}