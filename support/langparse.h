#pragma once

typedef int(*fnlangparse)();

struct langparsei {
	const char*	id;
	int			priority;
	fnlangparse	proc;
	size_t		size;
};