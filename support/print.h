#pragma once

typedef void(*fnoutput)(const char* format, const char* format_param);

extern fnoutput	print_proc;

void print(const char* format, ...);
void println();
void println(const char* format, ...);
void printv(const char* format, const char* format_param);
