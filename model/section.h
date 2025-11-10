#pragma once

enum sectionn {
	ModuleSection, LocalSection, DataSection, UDataSection, StringSection,
};
struct sectioni {
	int	ids; // section name
	void* data;
	size_t size, size_maximum;
	template<class T> void add(const T& v) { add(&v, sizeof(v)); }
	char* ptr(int offset) const;
	int addstring(const char* source, size_t len);
	void add(const void* source, size_t size);
	bool isvirtual() const { return size_maximum == -1; }
	void reserve(size_t size);
	void setvirtual() { size_maximum = -1; }
};
sectioni& section(int v);

int get_value(int sec, int offset, int size);

void initialize_sections();
void set_value(int sec, int offset, int size, int value);