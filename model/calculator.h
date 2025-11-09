#pragma once

enum operationn {
	Nop,
	Plus, Minus, Div, Mul, DivRest,
	BinaryOr, BinaryAnd, BinaryXor, Not, Neg,
	ShiftLeft, ShiftRight,
	Less, LessEqual, Greater, GreaterEqual, Equal, NotEqual,
	Or, And,
	Increment, Decrement, AdressOf, Dereference, Scope, Cast, Point,
	Assign, If, Else, While, Return, Switch, Case, Break, Continue, Default, In, Call,
	Number, Text, Identifier, Symbol, List,
};
enum symboln {
	Void, Auto, i8, u8, i16, u16, i32, u32, i64, u64, Bool,
};
enum symbolfn {
	Static, Private, Predefined, UseRead, UseWrite, Complete, Function,
};
enum scopen {
	TypeScope = -2, PointerScope = -3,
};
struct offseti {
	int			sid; // Section identifier, -1 for not instaced identifier
	int			offset; // Offset from section base
	void		clear() { sid = -1; offset = 0; }
	bool		needalloc() const { return sid != -1; }
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
	int			size; // Symbol size. Can be zero.
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

typedef void(*fnprint_scripter)(const char* format, const char* format_param);
extern fnprint_scripter scripter_error_proc;

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

int arifmetic(operationn op, int v1, int v2);
int ast_add(operationn op, int left, int right);
int const_number(int ast);
int create_symbol(int id, int type, unsigned flags, int scope, int parent);
int define_ast(int sid);
int dereference(int type);
int find_symbol(int ids, int scope, int parent);
int reference(int type);
int string_id(const char* name);
int symbol_ast(int sid);
int symbol_run(const char* symbol, const char* classid);
int symbol_scope(int sid);
int symbol_size(int sid);
int symbol_type(int sid);

offseti symbol_section(int sid);

const char* string_name(int sid);
const char* symbol_name(int sid);