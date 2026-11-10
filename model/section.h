#pragma once

enum sectionn : unsigned char {
	NoSection,
	ModuleSection, LocalSection, DataSection, UDataSection, StringSection,
	LastSection = StringSection
};

struct sectioni {
	int			ids; // section name
	void*		data;
	unsigned	size, size_maximum;
	sectioni() : ids(-1), data(0), size(0), size_maximum(0) {}
	constexpr explicit operator bool() const { return ids != -1; }
	template<class T> void add(const T& v) { add(&v, sizeof(v)); }
	unsigned addstring(const char* source, unsigned len);
	char* ptr(unsigned offset) const;
	void add(const void* source, unsigned size);
	bool isvirtual() const { return size_maximum == -1; }
	void reserve(unsigned size);
	void setvirtual() { size_maximum = -1; }
};
extern sectioni sections[LastSection + 1];

int get_value(sectionn sec, unsigned offset, unsigned size);

void initialize_sections();
void set_value(sectionn sec, unsigned offset, unsigned size, int value);