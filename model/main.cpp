#include "bsdata.h"
#include "calculator.h"
#include "io_stream.h"
#include "section.h"
#include "stringbuilder.h"

static const char* last_url_error;

void printcnf(const char* text);

static void println() {
	printcnf("\r\n");
}

static void printv(const char* format, const char* format_param) {
	char temp[512]; stringbuilder sb(temp);
	sb.addv(format, format_param);
	printcnf(temp);
}

static void printlnv(const char* format, const char* format_param) {
	char temp[512]; stringbuilder sb(temp);
	sb.addv(format, format_param);
	printcnf(temp);
	println();
}

static void print(const char* format, ...) {
	XVA_FORMAT(format);
	printv(format, format_param);
}

static void println(const char* format, ...) {
	XVA_FORMAT(format);
	printv(format, format_param);
	println();
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
		println("Module %1 (size %2i)", string_name(e.ids), e.instance.size);
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
		auto size = e.instance.size;
		if(size)
			print(" size(%1i)", size);
		auto frame = e.frame;
		if(frame)
			print(" frame(%1i)", frame);
		println();
	}
}

static void errorv(const char* url, const char* format, const char* format_param, const char* example) {
	if(last_url_error != url) {
		printcnf("Error when parsing `"); printcnf(url); printcnf("`");
		println();
		last_url_error = url;
	}
	printcnf(" "); printv(format, format_param);
	if(example) {
		printcnf(" in `"); printcnf(example); printcnf("`");
	}
	println();
}

static void initialize_parser() {
//	log::print_proc = printcnf;
	calculator_error_proc = errorv;
}

int main() {
	initialize_parser();
	initialize_sections();
	project_compile("code/test");
	if(iserrors())
		return -1;
	print_types();
	// symbol_run("run", "main");
	return 0;
}