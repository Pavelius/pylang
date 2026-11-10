#pragma once

#include "array.h"

struct stringa : public array {
	stringa() : array(1) {}
	unsigned add(const char* v);
	unsigned find(const char* v) const;
	const char*	get(unsigned v) const { return (v < count) ? (const char*)ptr(v) : ""; }
};

unsigned add_string(array& source, const char* value, size_t len);
unsigned find_string(const char* pb, const char* pe, const char* value, size_t len);