#pragma once

#include "array.h"

struct stringa : public array {
	stringa() : array(1) {}
	int add(const char* v);
	int find(const char* v) const;
	const char*	get(int v) const { return ((unsigned)v < (unsigned)count) ? (const char*)ptr(v) : ""; }
};

int add_string(array& source, const char* value, size_t len);
int find_string(const char* pb, const char* pe, const char* value, size_t len);