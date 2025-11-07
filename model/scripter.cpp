#include "bsdata.h"
#include "calculator.h"
#include "stringbuilder.h"

struct evaluei {
	int	type, value;
	void clear() { type = i32; value = 0; }
};
static evaluei operations[32];
static int operations_top;
fnprint_scripter scripter_error_proc;

static void error(const char* format, ...) {
	XVA_FORMAT(format);
	if(scripter_error_proc)
		scripter_error_proc(format, format_param);
}

static void pushv() {
	if(operations_top < lenghtof(operations) - 1)
		operations_top++;
	else
		error("Operation stack overflov.");
}

static void popv() {
	if(operations_top > 0)
		operations_top--;
	else
		error("Operation stack corrupt.");
}

static evaluei& get(int n) {
	return operations[operations_top + n];
}

static evaluei& get() {
	return operations[operations_top];
}

static int getv(int n = 0) {
	return get(n).value;
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

static void ast_run(int v) {
	if(v == -1)
		return;
	auto p = bsdata<asti>::begin() + v;
	switch(p->op) {
	//case If:
	//	ast_run(r, p->right);
	//	if(r.value)
	//		ast_run(e, p->left);
	//	break;
	//case While:
	//	while(true) {
	//		e.clear();
	//		ast_run(r, p->right);
	//		if(!e.value)
	//			break;
	//		r.clear();
	//		ast_run(r, p->left);
	//	}
	//	break;
	case Call:
		ast_run(p->right);
		ast_run(p->left);
		break;
	case Number:
		get().value = p->right;
		get().type = i32;
		pushv();
		break;
	case Symbol:
		ast_run(symbol_ast(p->right));
		break;
	case Text:
		get().value = p->right;
		get().type = get_string_type();
		pushv();
		break;
	case Assign:
		ast_run(p->right);
		ast_run(p->left);
		break;
	case List:
		ast_run(p->left);
		ast_run(p->right);
		break;
	case Return:
		ast_run(p->right);
		break;
	default:
		break;
	}
}

int const_number(int ast) {
	if(ast == -1)
		return 0;
	ast_run(ast);
	if(get().type != i32)
		error("Must be constant expression");
	return get().value;
}

int symbol_run(const char* symbol, const char* classid) {
	auto tid = find_symbol(string_id(classid), TypeScope, 0);
	if(tid == -1)
		return -1;
	auto sid = find_symbol(string_id(symbol), 0, tid);
	if(sid == -1)
		return -1;
	ast_run(symbol_ast(sid));
	popv();
	return get().value;
}