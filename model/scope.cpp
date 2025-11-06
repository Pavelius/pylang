#include "scope.h"

scopei*	current_scope;
int		scope_maximum;
int		scope_ident;

scopei::scopei(int value) {
	scope = value;
	size = 0;
	previous = current_scope;
	current_scope = this;
	scope_ident++;
}

scopei::~scopei() {
	auto n = getsize();
	if(scope_maximum < n)
		scope_maximum = n;
	current_scope = previous;
	scope_ident--;
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
	scope_ident = 0;
}