#include "bsdata.h"
#include "calculator.h"
#include "io_stream.h"
#include "print.h"
#include "section.h"
#include "stringbuilder.h"

static const char* last_url_error;

void printcnf(const char* text);

static void printcnf_proc(const char* format, const char* format_param) {
	if(format_param) {
		char temp[512]; stringbuilder sb(temp);
		sb.addv(format, format_param);
		printcnf(temp);
	} else
		printcnf(format);
}

void symbol_info(stringbuilder& sb, const symboli& e) {
	auto sid = e.getindex();
	if(e.type != -1) {
		auto& et = bsdata<symboli>::get(e.type);
		sb.add(string_name(et.ids));
		sb.add(" ");
	}
	sb.add(string_name(e.ids));
	if(e.is(Function)) {
		sb.add("(");
		auto param_count = 0;
		for(auto& m : bsdata<symboli>()) {
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
	for(auto& e : bsdata<symboli>()) {
		if(e.ispredefined() || e.type == -1 || e.ispointer() || !e.istype())
			continue;
		auto sid = &e - bsdata<symboli>::begin();
		println("Module %1 (size %2i)", string_name(e.ids), e.size);
		for(auto& m : bsdata<symboli>()) {
			if(m.ispredefined() || m.type == -1 || m.parent != sid)
				continue;
			sb.clear(); symbol_info(sb, m);
			println(sb);
		}
	}
}

static void print_symbols() {
	for(auto& e : bsdata<symboli>()) {
		if(e.ispredefined() || e.type == -1)
			continue;
		auto pt = bsdata<symboli>::begin() + e.type;
		auto sid = e.getindex();
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
		println("Result string `%1`", section(StringSection).ptr(test_value));
	else
		println("Result number is %1i", test_value);
	return 0;
}