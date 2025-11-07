#include "print.h"
#include "stringbuilder.h"

fnoutput print_proc;

void printv(const char* format, const char* format_param) {
	if(print_proc)
		print_proc(format, format_param);
}

void print(const char* format, ...) {
	XVA_FORMAT(format);
	printv(format, format_param);
}

void println(const char* format, ...) {
	XVA_FORMAT(format);
	printv(format, format_param);
	println();
}

void println() {
	printv("\r\n", 0);
}