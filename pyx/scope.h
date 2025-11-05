#pragma once

struct scopei {
	scopei* previous;
	int		scope;
	int		size;
	scopei(int value);
	~scopei();
	int		getsize() const;
};
extern scopei* current_scope;
extern int scope_maximum;

void scope_clear();
