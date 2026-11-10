#pragma once

struct scopei {
	scopei*		previous;
	int			scope;
	unsigned	size;
	scopei(int value);
	~scopei();
};
extern scopei* current_scope;
extern unsigned scope_maximum;
extern unsigned scope_ident;

void scope_clear();

unsigned scope_size();
