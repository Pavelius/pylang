#pragma once

enum sectionn {
	ModuleSection, LocalSection, DataSection, UDataSection,
};
struct sectioni {
	int		ids; // section name
	void*	data;
	size_t	size, size_maximum;
	template<class T> void add(const T& v) { add(&v, sizeof(v)); }
	char* ptr(int offset) const;
	int getvalue(int offset, int size) const;
	void add(const void* source, size_t size);
	bool isvirtual() const { return size_maximum == -1; }
	void reserve(size_t size);
	void setvalue(int offset, int size, int value) const;
	void setvirtual() { size_maximum = -1; }
};
sectioni& section(int v);

void initialize_sections();