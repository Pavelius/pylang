#include "stringbuilder.h"
#include "win.h"

void printcnf(const char* text) {
	unsigned result;
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text, zlen(text), &result, 0);
}