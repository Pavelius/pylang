#include "bsdata.h"
#include "calculator.h"

struct evaluei {
	int	type, value;
	void clear() { type = i32; value = 0; }
};

static void error(const char* format, ...) {
}

static int get_string_type() {
	static int r = -1;
	if(r == -1)
		r = reference(i8);
	return r;
}

static const asti* get_ast(int v) {
	if(v == -1)
		return 0;
	return bsdata<asti>::begin() + v;
}

static bool isnumber(int type) {
	switch(type) {
	case i8: case u8: case i16: case u16: case i32: case u32: return true;
	default: return false;
	}
}

static void dereference(evaluei* p) {
}

static void binary_operation(operationn op, evaluei& e1, evaluei& e2) {
	if(isnumber(e1.type) && isnumber(e2.type))
		e1.value = arifmetic(op, e1.value, e2.value);
	else if(isnumber(e2.type))
		e1.value = arifmetic(op, e1.value, e2.value * symbol_size(dereference(e1.type)));
}

static void ast_run(evaluei& e, int v) {
	if(v == -1)
		return;
	evaluei r;
	auto p = bsdata<asti>::begin() + v;
	switch(p->op) {
	case If:
		r.clear();
		ast_run(r, p->right);
		if(r.value)
			ast_run(e, p->left);
		break;
	case While:
		while(true) {
			e.clear();
			ast_run(r, p->right);
			if(!e.value)
				break;
			r.clear();
			ast_run(r, p->left);
		}
		break;
	case Call:
		ast_run(e, p->right);
		ast_run(e, p->left);
		break;
	case Number:
		e.type = i32;
		e.value = p->right;
		break;
	case Identifier:
		ast_run(e, symbol_ast(p->right));
		break;
	case Text:
		e.type = get_string_type();
		e.value = p->right;
		break;
	case Assign:
		r.clear();
		ast_run(r, p->right);
		ast_run(e, p->left);
		break;
	case List:
		ast_run(e, p->left);
		ast_run(e, p->right);
		break;
	default:
		r.clear();
		ast_run(r, p->right);
		ast_run(e, p->left);
		binary_operation(p->op, e, r);
		break;
	}
}

int const_number(int ast) {
	if(ast == -1)
		return 0;
	evaluei e = {};
	ast_run(e, ast);
	if(e.type != i32)
		error("Must be constant expression");
	return e.value;
}

int symbol_run(const char* symbol, const char* classid) {
	auto tid = find_symbol(string_id(classid), TypeScope, 0);
	if(tid == -1)
		return -1;
	auto sid = find_symbol(string_id(symbol), 0, tid);
	if(sid == -1)
		return -1;
	evaluei e;
	ast_run(e, symbol_ast(sid));
	return e.value;
}