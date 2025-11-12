#include "array.h"
#include "math.h"
#include "stringbuilder.h"

extern "C" void exit(int exit_code) noexcept;
extern "C" void* malloc(long unsigned size);
extern "C" void* realloc(void *ptr, long unsigned size);
extern "C" void	free(void* pointer);

unsigned rmoptimal(unsigned need_count) {
	const unsigned mc = 4 * 256 * 256;
	unsigned m = 16;
	while(m < mc) {
		m = m << 1;
		if(need_count < m)
			return m;
	}
	return need_count + mc;
}

void* realloc_data(void* data, size_t new_size) {
	if(data) {
		auto p = realloc(data, new_size);
		if(!p)
			exit(0);
		return p;
	} else
		return malloc(new_size);
}

void* realloc_data(void* data, size_t new_size, size_t& new_size_maximum) {
	if(new_size <= new_size_maximum)
		return data;
	new_size_maximum = rmoptimal(new_size);
	return realloc_data(data, new_size);
}

void* find_data(void* source_data, size_t source_count, const void* data, size_t count) {
	if(!source_data || !data)
		return 0;
	if(count > source_count)
		return 0;
	auto ch = *((unsigned char*)data);
	if(count==1)
		return memchr(source_data, ch, source_count);
	auto pb = (unsigned char*)source_data;
	auto pe = pb + (source_count - count);
	auto ps = (unsigned char*)data + 1;
	auto n = count - 1;
	while(pb < pe) {
		pb = (unsigned char*)memchr(pb, ch, pe - pb);
		if(!pb)
			break;
		if(memcmp(pb+1, ps, n)==0)
			return pb;
		pb++;
	}
	return 0;
}

void* array::add() {
	if(count >= getmaximum()) {
		if(isgrowable())
			reserve(count + 1);
		else
			return data;
	}
	return (char*)data + element_size * (count++);
}

void* array::add(const void* element) {
	auto p = add();
	memcpy(p, element, size());
	return p;
}

void* array::addfind(const char* id) {
	auto p = findv(id, 0);
	if(p)
		return p;
	p = add();
	*((const char**)p) = id;
	return p;
}

array::~array() {
	clear();
}

void array::clear() {
	count = 0;
	if(!isgrowable())
		return;
	count_maximum = 0;
	if(data)
		free(data);
	data = 0;
}

void array::setup(size_t size) {
	if(!isgrowable())
		return;
	clear();
	this->element_size = size;
}

void array::reserve(unsigned count) {
	if(!isgrowable())
		return;
	if(!element_size)
		return;
	if(data && count < getmaximum())
		return;
	count_maximum = rmoptimal(count);
	data = realloc_data(data, count_maximum * element_size);
}

void* array::findv(const char* value, unsigned offset, size_t string_size) const {
	if(!data)
		return 0;
	auto pe = end();
	for(auto p = begin() + offset; p < pe; p += element_size) {
		auto pn = *((const char**)p);
		if(!pn)
			continue;
		if(memcmp(pn, value, string_size) == 0)
			return p;
	}
	return 0;
}

void* array::findv(const char* id, unsigned offset) const {
	return findv(id, offset, zlen(id) + 1);
}

int array::find(int i1, int i2, void* value, unsigned offset, size_t size) const {
	if(i2 == -1)
		i2 = getcount() - 1;
	switch(size) {
	case 4:
		for(auto i = i1; i <= i2; i++) {
			if(*((int*)value) == *((int*)((char*)ptr(i) + offset)))
				return i;
		}
		break;
	case 2:
		for(auto i = i1; i <= i2; i++) {
			if(*((int*)value) == *((int*)((char*)ptr(i) + offset)))
				return i;
		}
		break;
	case 1:
		for(auto i = i1; i <= i2; i++) {
			if(*((int*)value) == *((int*)((char*)ptr(i) + offset)))
				return i;
		}
		break;
	default:
		for(auto i = i1; i <= i2; i++) {
			if(memcmp(value, (char*)ptr(i) + offset, size) == 0)
				return i;
		}
		break;
	}
	return -1;
}

void array::remove(int index, int elements_count) {
	if(((unsigned)index) >= count)
		return;
	if((unsigned)index < count - elements_count)
		memcpy(ptr(index), ptr(index + elements_count), (count - (index + elements_count)) * size());
	count -= elements_count;
}

int	array::indexof(const void* element) const {
	if(element >= data && element < ((char*)data + element_size * count))
		return ((char*)element - (char*)data) / element_size;
	return -1;
}

void* array::insert(int index, const void* element) {
	auto count_before = getcount(); add();
	memmove((char*)data + (index + 1) * element_size, (char*)data + index * element_size, (count_before - index) * element_size);
	void* p = ptr(index);
	if(element)
		memcpy(p, element, element_size);
	else
		memset(p, 0, element_size);
	return p;
}

void array::swap(int i1, int i2) {
	unsigned char* a1 = (unsigned char*)ptr(i1);
	unsigned char* a2 = (unsigned char*)ptr(i2);
	for(unsigned i = 0; i < element_size; i++) {
		char a = a1[i];
		a1[i] = a2[i];
		a2[i] = a;
	}
}

void array::shift(int i1, int i2, size_t c1, size_t c2) {
	if(i2 < i1) {
		iswap(i2, i1);
		iswap(c1, c2);
	}
	auto a1 = (char*)ptr(i1);
	auto a2 = (char*)ptr(i2);
	auto s1 = c1 * element_size;
	auto s2 = c2 * element_size;
	unsigned s = (a2 - a1) + s2 - 1;
	for(unsigned i = 0; i < s1; i++) {
		auto a = a1[0];
		memcpy(a1, a1 + 1, s);
		a1[s] = a;
	}
}

void array::shrink(unsigned offset, size_t delta) {
	if(offset + delta > element_size)
		return;
	auto p1 = (char*)data;
	auto p2 = (char*)data;
	const auto s2 = element_size - delta - offset;
	auto pe = p1 + count * element_size;
	while(p1 < pe) {
		if(offset) {
			memcpy(p2, p1, offset);
			p2 += offset; p1 += offset;
		}
		p1 += delta;
		if(s2) {
			memcpy(p2, p1, s2);
			p2 += s2; p1 += s2;
		}
	}
	auto new_size = element_size - delta;
	count_maximum = getmaximum() * element_size / new_size;
	element_size = new_size;
}

void array::grow(unsigned offset, size_t delta) {
	if(!delta)
		return;
	if(!isgrowable())
		return;
	auto new_size = element_size + delta;
	auto new_size_bytes = count_maximum * new_size;
	if(data) {
		auto p = realloc(data, new_size_bytes);
		if(p)
			data = p;
	} else
		data = malloc(new_size_bytes);
	auto p1 = (char*)data + new_size * count;
	auto p2 = (char*)data + element_size * count;
	auto s2 = element_size - offset;
	while(p1 > data) {
		if(s2) {
			memcpy(p1 - s2, p1 - s2, s2);
			p2 -= s2; p1 -= s2;
		}
		p1 -= delta;
		if(offset) {
			memcpy(p1, p2, offset);
			p2 -= offset; p1 -= offset;
		}
	}
	element_size = new_size;
}

void array::zero(unsigned offset, size_t delta) {
	if(!delta)
		return;
	auto pe = (char*)data + element_size * count;
	for(auto p = (char*)data + offset; p < pe; p += element_size)
		memset(p, 0, delta);
}

void array::change(unsigned offset, int size) {
	if(!size)
		return;
	if(size > 0) {
		grow(offset, size);
		zero(offset, size);
	} else
		shrink(offset, -size);
}

const void* array::findu(const void* value, size_t size) const {
	return find_data(data, count * element_size, value, size);
}

void* array::addu(const void* element, unsigned count) {
	auto s = count * element_size;
	auto p = findu(element, s);
	if(p)
		return (void*)p;
	auto ci = this->count;
	reserve(this->count + count);
	auto pr = ptr(ci);
	memcpy(pr, element, s);
	this->count += count;
	return pr;
}

const char* array::findus(const char* value, size_t size) const {
	auto p = begin();
	auto pe = end();
	auto s = *value;
	while(p < pe) {
		p = (char*)memchr(p, s, pe - p);
		if(!p || (unsigned(pe - p) < size))
			break;
		if(memcmp(p, value, size) == 0 && p[size] == 0)
			return p;
		p++;
	}
	return 0;
}

const char* array::addus(const char* element, unsigned count) {
	auto p = findus(element, count);
	if(p)
		return p;
	auto ci = this->count;
	reserve(this->count + count + 1);
	auto pr = (char*)ptr(ci);
	memcpy(pr, element, count);
	pr[count] = 0;
	this->count += count + 1;
	return pr;
}

void array::repack(unsigned& start, unsigned count) {
	if(!count || (start + count != this->count))
		return;
	auto i = indexof(findu(ptr(start), count * element_size));
	if(i == -1 || (unsigned)i >= start)
		return;
	start = i;
	this->count = start + count;
}