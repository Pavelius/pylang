#pragma once

enum operationn {
	Nop,
	Plus, Minus, Div, Mul, DivRest,
	BinaryOr, BinaryAnd, BinaryXor, Not, Neg,
	ShiftLeft, ShiftRight,
	Less, LessEqual, Greater, GreaterEqual, Equal, NotEqual,
	Or, And,
	Increment, Decrement, AdressOf, Dereference, Scope, Cast, Point,
	Assign, If, While, Return, Switch, Case, Break, Continue, Default,
	Number, Text, Identifier, List, Initialization,
};
enum symboln {
	Void, i8, u8, i16, u16, i32, u32, i64, u64,
};
enum symbolfn {
	Static, Public, Predefined, UseRead, UseWrite, Complete, Function,
};
enum scopen {
	TypeScope = -2, PointerScope = -3,
};
struct offseti {
	int			sid; // section identifier, -1 for not instaced identifier
	int			offset; // offset from section base
	int			size; // symbol size
	bool		needalloc() const { return sid != -1; }
	void*		ptr() const { return 0; }
};
struct symboli {
	int			ids; // string id, use string_name() to get text
	int			scope; // visibility scope (-2 for pointers, -3 for platform externs)
	int			parent; // parent symbol. Module or member.
	int			type; // other symbol id
	int			value; // initialization value
	int			count; // 0 - default, 1+ for array
	unsigned	flags; // Some flags
	int			frame; // symbol stack frame
	offseti		instance; // symbol instance in memory
	int			getindex() const;
	bool		is(symbolfn v) const { return (flags & (1 << v)) != 0; }
	bool		isarray() const { return count >= 0; }
	bool		istype() const { return scope == TypeScope || scope == PointerScope; }
	bool		ispointer() const { return scope == PointerScope; }
	bool		ispredefined() const { return is(Predefined); }
	void		set(symbolfn v) { flags |= (1 << v); }
};
struct asti {
	operationn	op;
	int			left;
	int			right;
};
struct definei {
	int			ids;
	int			ast;
};

typedef void(*calculator_fnprint)(const char* header, const char* format, const char* format_param, const char* example);
extern calculator_fnprint calculator_error_proc;

void calculator_file_parse(const char* url);
void calculator_parse(const char* code);
bool isbinary(operationn op);
bool iserrors();
bool isterminal(operationn op);
void project_compile(const char* url);
bool symbol(int sid, symbolfn v);
void symbol_ast(int sid, int value);
void symbol_count(int sid, int value);
void symbol_scope(int sid, int value);
void symbol_set(int sid, symbolfn v);
void symbol_type(int sid, int value);

int ast_add(operationn op, int left, int right);
int const_number(int ast);
int define_ast(int sid);
int dereference(int type);
int find_symbol(int ids, int scope, int parent);
int predefined_symbol_size(int type);
int reference(int type);
int symbol_ast(int sid);
int symbol_scope(int sid);
int symbol_size(int sid);
int symbol_type(int sid);

offseti symbol_section(int sid);

const char* string_name(int sid);
const char* symbol_name(int sid);