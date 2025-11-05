#include "scope.h"

scopei*	current_scope;
int		scope_maximum;

scopei::scopei(int value) {
	scope = value;
	size = 0;
	previous = current_scope;
	current_scope = this;
}

scopei::~scopei() {
	auto n = getsize();
	if(scope_maximum < n)
		scope_maximum = n;
	current_scope = previous;
}

int scopei::getsize() const {
	auto r = 0;
	for(auto p = current_scope; p; p = p->previous)
		r += p->size;
	return r;
}

void scope_clear() {
	current_scope = 0;
	scope_maximum = 0;
}