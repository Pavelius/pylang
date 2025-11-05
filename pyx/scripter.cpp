#include "bsdata.h"
#include "calculator.h"

struct evaluei {
	int		type;
	int		value;
	void	clear() { type = i32; value = 0; }
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

static int arifmetic(operationn op, int v1, int v2) {
	switch(op) {
	case Plus: return v1 + v2;
	case Minus: return v1 - v2;
	case Mul: return v1 * v2;
	case Div: return v1 / v2;
	case DivRest: return v1 % v2;
	case BinaryOr: return v1 | v2;
	case BinaryAnd: return v1 & v2;
	case BinaryXor: return v1 ^ v2;
	case ShiftLeft: return v1 << v2;
	case ShiftRight: return v1 >> v2;
	case Less: return v1 < v2;
	case LessEqual: return v1 <= v2;
	case Greater: return v1 > v2;
	case GreaterEqual: return v1 >= v2;
	case Equal: return v1 == v2;
	case NotEqual: return v1 != v2;
	case Or: return (v1 || v2) ? 1 : 0;
	case And: return (v1 && v2) ? 1 : 0;
	case Neg: return -v1;
	case Not: return v1 ? 0 : 1;
	default: return v1;
	}
}

static void binary_operation(operationn op, evaluei& e1, evaluei& e2) {
	if(isnumber(e1.type) && isnumber(e2.type))
		e1.value = arifmetic(op, e1.value, e2.value);
	else if(isnumber(e2.type))
		e1.value = arifmetic(op, e1.value, e2.value * symbol_size(dereference(e1.type)));
}

static void run_ast(evaluei& e, int v) {
	if(v == -1)
		return;
	evaluei r;
	auto p = bsdata<asti>::begin() + v;
	switch(p->op) {
	case If:
		r.clear();
		run_ast(r, p->right);
		if(r.value)
			run_ast(e, p->left);
		break;
	case While:
		while(true) {
			e.clear();
			run_ast(r, p->right);
			if(!e.value)
				break;
			r.clear();
			run_ast(r, p->left);
		}
		break;
	case Number:
		e.type = i32;
		e.value = p->right;
		break;
	case Identifier:
		run_ast(e, symbol_ast(p->right));
		break;
	case Text:
		e.type = get_string_type();
		e.value = p->right;
		break;
	case Assign:
		r.clear();
		run_ast(r, p->right);
		run_ast(e, p->left);
		break;
	default:
		r.clear();
		run_ast(r, p->right);
		run_ast(e, p->left);
		binary_operation(p->op, e, r);
		break;
	}
}

int const_number(int ast) {
	if(ast == -1)
		return 0;
	evaluei e = {};
	run_ast(e, ast);
	if(e.type != i32)
		error("Must be constant expression");
	return e.value;
}