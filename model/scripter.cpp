#include "bsdata.h"
#include "calculator.h"
#include "stringbuilder.h"
#include "section.h"

struct evaluei {
	int	type;
	int value;
	int sid; // Symbol id for rvalue conversion. -1 if no rvalue.
	int sec; // Section id. -1 for none.
	void clear() { type = i32; value = 0; sec = -1; sid = -1; }
	bool islvalue() const { return sid != -1; }
	bool isrvalue() const { return sid == -1; }
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

static void binary_operation(operationn op) {
	evaluei& e1 = get(-2);
	evaluei& e2 = get(-1);
	if(isnumber(e1.type) && isnumber(e2.type))
		e1.value = arifmetic(op, e1.value, e2.value);
	popv();
}

static void push_number(int value) {
	get().clear();
	get().value = value;
	get().type = i32;
	pushv();
}

static void push_literal(int ids) {
	get().clear();
	get().value = ids;
	get().type = get_string_type();
	pushv();
}

static void push_symbol(int sid) {
	auto& e = get();
	e.clear();
	e.sid = sid;
	e.value = symbol_section(sid).offset;
	e.type = reference(symbol_type(sid));
	e.sec = symbol_section(sid).sid;
	pushv();
}

static int get_value(int sid, int offset, int type) {
	if(sid == -1)
		return 0;
	auto p = bsdata<sectioni>::begin() + sid;
	return p->getvalue(offset, symbol_size(type));
}

static void set_value(int sid, int offset, int type, int value) {
	if(sid == -1)
		return;
	auto p = bsdata<sectioni>::begin() + sid;
	p->setvalue(offset, symbol_size(type), value);
}

static void rvalue() {
	auto& e = get(-1);
	if(!e.isrvalue()) {
		e.type = dereference(e.type);
		e.value = get_value(e.sec, e.value, e.type);
		e.sid = -1;
		e.sec = -1;
	}
}

static void assignment(evaluei& e1, evaluei& e2) {
}

static void ast_run(int v) {
	if(v == -1)
		return;
	auto p = bsdata<asti>::begin() + v;
	switch(p->op) {
	case If:
		ast_run(p->right);
		rvalue();
		popv();
		if(get().value)
			ast_run(p->left);
		break;
	case While:
		while(true) {
			ast_run(p->right);
			rvalue();
			popv();
			if(!get().value)
				break;
			ast_run(p->left);
		}
		break;
	case Call:
		ast_run(p->right);
		ast_run(p->left);
		break;
	case Number:
		push_number(p->right);
		break;
	case Text:
		push_literal(p->right);
		break;
	case Symbol:
		push_symbol(p->right);
		break;
	case Assign:
		ast_run(p->right);
		ast_run(p->left);
		rvalue();
		assignment(get(-2), get(-1));
		popv();
		popv();
		break;
	case List:
		ast_run(p->left);
		ast_run(p->right);
		break;
	case Return:
		ast_run(p->right);
		break;
	case Plus: case Minus: case Div: case Mul: case DivRest:
	case BinaryOr: case BinaryAnd: case BinaryXor:
	case ShiftLeft: case ShiftRight:
		ast_run(p->left);
		rvalue();
		ast_run(p->right);
		rvalue();
		binary_operation(p->op);
		break;
	case Neg: case Not:
		break;
	default:
		break;
	}
}

int const_number(int ast) {
	if(ast == -1)
		return 0;
	ast_run(ast);
	popv();
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
	return get(-1).value;
}