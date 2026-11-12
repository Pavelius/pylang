#include "calculator.h"
#include "io_stream.h"
#include "print.h"
#include "section.h"
#include "stringbuilder.h"
#include "vector.h"

static const char* last_url_error;

void printcnf(const char* text);

extern vector<symboli> symbols;

static void printcnf_proc(const char* format, const char* format_param) {
	if(format_param) {
		char temp[512]; stringbuilder sb(temp);
		sb.addv(format, format_param);
		printcnf(temp);
	} else
		printcnf(format);
}

static void symbol_info(stringbuilder& sb, const symboli& e) {
	auto sid = e.index();
	if(e.type != -1) {
		sb.add(symbol_name(e.type));
		sb.add(" ");
	}
	sb.add(string_name(e.ids));
	if(e.is(Function)) {
		sb.add("(");
		auto param_count = 0;
		for(auto& m : symbols) {
			if(m.parent != sid)
				continue;
			if(param_count > 0)
				sb.add(", ");
			symbol_info(sb, m);
			param_count++;
		}
		sb.add(")");
	}
}

static void print_types() {
	char temp[260]; stringbuilder sb(temp);
	for(auto& e : symbols) {
		if(e.ispredefined() || e.type == -1 || e.ispointer() || !e.istype())
			continue;
		auto sid = &e - symbols.begin();
		println("module %1 (size %2i)", symbol_name(sid), e.size);
		for(auto& m : symbols) {
			if(m.ispredefined() || m.type == -1 || m.parent != sid)
				continue;
			sb.clear(); symbol_info(sb, m);
			print(" ");
			println(sb);
		}
	}
}

static void print_symbols() {
	for(auto& e : symbols) {
		if(e.ispredefined() || e.type == -1)
			continue;
		auto pt = symbols.begin() + e.type;
		auto sid = e.index();
		print("%2 %1", string_name(e.ids), string_name(pt->ids));
		auto size = e.size;
		if(size)
			print(" size(%1i)", size);
		auto frame = e.frame;
		if(frame)
			print(" frame(%1i)", frame);
		println();
	}
}

int main() {
	print_proc = printcnf_proc;
	initialize_sections();
	project_compile("code/test");
	if(iserrors())
		return -1;
	print_types();
	auto test_value = symbol_run("run", "main");
	if(operation_type(0)==string_type)
		println("Result string `%1`", sections[StringSection].ptr(test_value));
	else
		println("Result number is %1i", test_value);
	return 0;
}