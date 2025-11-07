#pragma once

#include "array.h"

class stringa : public array {
	int	find(const char* v, size_t len) const;
	int add(const char* v, size_t len);
public:
	stringa() : array(1) {}
	int add(const char* v);
	int find(const char* v) const;
	const char*	get(int v) const { return ((unsigned)v < (unsigned)count) ? (const char*)ptr(v) : ""; }
};
