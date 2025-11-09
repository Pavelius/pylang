#include "bsdata.h"
#include "calculator.h"
#include "flagable.h"
#include "io_stream.h"
#include "print.h"
#include "scope.h"
#include "section.h"
#include "stringbuilder.h"
#include "stringa.h"

BSDATAD(asti)
BSDATAD(definei)
BSDATAD(symboli)

static int function_params;
static int last_type;
static int last_ident;
static unsigned last_flags;
static long last_number;
static bool need_return;

static const char* project_url;
const char*	library_url;

static const char* p;
static const char* p_start;
static const char* p_start_line;
static const char* p_url;
static const char* last_url_error;
static char	last_string[256 * 256];

static stringa strings;

static int errors_count;

static int module_sid;

static int size_of_pointer = 4;

static int parse_expression();
static int unary();
static int expression();

struct pushparam {
	int params, scope;
	pushparam(int sid) : params(function_params), scope(scope_maximum) { scope_maximum = 0; function_params = sid; }
	~pushparam() { function_params = params; scope_maximum = scope; }
};
struct pushfile {
	const char* pointer;
	const char* start;
	const char* start_line;
	const char* url;
	int ident;
	pushfile(const char* p1) : pointer(p), start(p_start), start_line(p_start_line), url(p_url), ident(last_ident) {
		p = p1;
		p_start = p1;
		p_start_line = p1;
		last_ident = 0;
	}
	~pushfile() { p = pointer; p_start = start; p_start_line = start_line; p_url = url; last_ident = ident; }
};

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

static const char* create_example() {
	static char temp[260]; stringbuilder sb(temp); sb.clear();
	if(!p)
		return 0;
	auto p1 = p;
	// Find line begin
	while(p1 > p_start) {
		if(p1[-1] == 10 || p1[-1] == 13 || p1[-1] == 9)
			break;
		p1--;
	}
	// Skip until line end
	while(*p1 && *p1 != 10 && *p1 != 13) {
		sb.add(*p1);
		p1++;
		if(sb.isempthy())
			break;
	}
	return temp;
}

static void error(const char* format, ...) {
	errors_count++;
	XVA_FORMAT(format);
	errorv(p_url, format, format_param, create_example());
}

bool isterminal(operationn v) {
	switch(v) {
	case Number: case Text: case Identifier: case Symbol: return true;
	case Continue: case Break: return true;
	default: return false;
	}
}

bool isbinary(operationn op) {
	switch(op) {
	case Plus: case Minus: case Div: case Mul: case DivRest:
	case BinaryOr: case BinaryAnd: case BinaryXor:
	case ShiftLeft: case ShiftRight:
	case Less: case LessEqual: case Greater: case GreaterEqual: case Equal: case NotEqual:
	case Or: case And:
	case Scope: case Cast: case Point:
		return true;
	default: return false;
	}
}

bool isstrict(operationn op) {
	switch(op) {
	case Plus: case Mul: case BinaryOr: case BinaryAnd: case BinaryXor: return false;
	default: return true;
	}
}

bool iserrors() {
	return errors_count > 0;
}

int define_ast(int sid) {
	if(sid == -1)
		return -1;
	return bsdata<definei>::get(sid).ast;
}

int symboli::getindex() const {
	return this - bsdata<symboli>::begin();
}

bool symbol(int sid, symbolfn v) {
	if(sid == -1)
		return false;
	return bsdata<symboli>::get(sid).is(v);
}

const char* symbol_name(int sid) {
	if(sid == -1)
		return "";
	auto& e = bsdata<symboli>::get(sid);
	return strings.get(e.ids);
}

void symbol_set(int sid, symbolfn v) {
	if(sid == -1)
		return;
	bsdata<symboli>::get(sid).set(v);
}

int symbol_ast(int sid) {
	if(sid == -1)
		return -1;
	return bsdata<symboli>::get(sid).value;
}

void symbol_ast(int sid, int value) {
	if(sid == -1)
		return;
	bsdata<symboli>::get(sid).value = value;
}

int symbol_type(int sid) {
	if(sid == -1)
		return -1;
	auto n = bsdata<symboli>::get(sid).type;
	if(n == -1)
		return sid;
	return n;
}

int symbol_size(int sid) {
	if(sid == -1)
		return 0;
	return bsdata<symboli>::get(sid).size;
}

void symbol_frame(int sid, int value) {
	if(sid == -1)
		return;
	bsdata<symboli>::get(sid).frame = value;
}

offseti symbol_section(int sid) {
	if(sid == -1)
		return {};
	return bsdata<symboli>::get(sid).instance;
}

void symbol_type(int sid, int value) {
	if(sid == -1)
		return;
	bsdata<symboli>::get(sid).type = value;
}

void symbol_count(int sid, int value) {
	if(sid == -1)
		return;
	auto& e = bsdata<symboli>::get(sid);
	e.count = value;
}

int symbol_scope(int sid) {
	if(sid == -1)
		return -1;
	return bsdata<symboli>::get(sid).scope;
}

void symbol_scope(int sid, int value) {
	if(sid == -1)
		return;
	auto& e = bsdata<symboli>::get(sid);
	e.scope = value;
}

const char* symbol_name(int sid, int value) {
	if(sid == -1)
		return "unknown";
	return string_name(bsdata<symboli>::get(sid).ids);
}

static int getscope() {
	if(current_scope)
		return current_scope->scope;
	return 0;
}

static void create_instance(int sec, int type, int offset, int ast) {
	auto& e = bsdata<symboli>::get(type);
	set_value(sec, offset, symbol_size(type), const_number(ast));
}

static void instance_symbol(int sid, int section_id) {
	if(sid == -1 || section_id == -1)
		return;
	auto& e = bsdata<symboli>::get(sid);
	if(e.instance.sid != -1)
		return;
	e.instance.sid = section_id;
	if(section_id == LocalSection) {
		if(current_scope) {
			e.instance.offset = current_scope->getsize();
			current_scope->size += e.size;
		}
	} else if(section_id == ModuleSection) {
		auto& module = bsdata<symboli>::get(module_sid);
		e.instance.offset = module.size;
		module.size += e.size;
	} else {
		auto& s = bsdata<sectioni>::get(section_id);
		e.instance.offset = s.size;
		s.reserve(e.size);
		s.size += e.size;
		create_instance(section_id, e.type, e.instance.offset, e.value);
	}
}

const char* string_name(int sid) {
	if(sid == -1)
		return "";
	return strings.get(sid);
}

int string_id(const char* name) {
	if(!name || name[0] == 0)
		return -1;
	return strings.add(name);
}

static void skipws() {
	while(true) {
		if(*p == ' ' || *p == 9) {
			p++;
			continue;
		}
		if(p[0] == '#') { // End line comment
			p += 1;
			while(*p && *p != 10 && *p != 13)
				p++;
			continue;
		}
		break;
	}
}

static void skipws(int count) {
	p += count;
	skipws();
}

static void skip_line_feed() {
	while(p[0] == 10 || p[0] == 13) {
		while(p[0] == 10 || p[0] == 13)
			p = skipcr(p);
		auto pb = p;
		while(*p == ' ')
			p++;
		last_ident = p - pb;
		p_start_line = p;
		skipws();
	}
}

static bool match(const char* symbol) {
	auto i = 0;
	while(symbol[i]) {
		if(p[i] != symbol[i])
			return false;
		i++;
	}
	p += i;
	skipws();
	return true;
}

static bool word(const char* symbol) {
	auto i = 0;
	while(symbol[i]) {
		if(p[i] != symbol[i])
			return false;
		i++;
	}
	auto c = p[i];
	if(ischa(c) || isnum(c))
		return false; // This is identifier, not word
	p += i;
	skipws();
	return true;
}

static void skip(const char* symbol) {
	if(!match(symbol))
		error("Expected token `%1`", symbol);
}

static bool next_statement() {
	if(!p[0])
		return false;
	if(match(";")) {
		skipws();
		return true;
	}
	skip_line_feed();
	if(last_ident > scope_ident)
		error("Expected ident %1i spaces", scope_ident);
	else if(last_ident < scope_ident)
		return false;
	return true;
}

static bool isidentifier() {
	return *p == '_' || ischa(*p);
}

int find_symbol(int ids, int scope, int parent) {
	for(auto& e : bsdata<symboli>()) {
		if(e.ids == ids && e.scope == scope && e.parent == parent)
			return e.getindex();
	}
	return -1;
}

static int find_define(int ids) {
	for(auto& e : bsdata<definei>()) {
		if(e.ids == ids)
			return &e - bsdata<definei>::begin();
	}
	return -1;
}

static int find_section(int ids) {
	for(auto& e : bsdata<sectioni>()) {
		if(e.ids == ids)
			return &e - bsdata<sectioni>::begin();
	}
	return -1;
}

static void find_type(int ast, int& result) {
	if(ast == -1 || result != -1)
		return;
	auto p = bsdata<asti>::begin() + ast;
	if(p->op == Identifier) {
		auto type = symbol_type(p->right);
		if(type != -1)
			result = type;
	} else if(isbinary(p->op)) {
		find_type(p->right, result);
		find_type(p->left, result);
	} else if(isterminal(p->op)) {
		// Nothing to do
	} else
		find_type(p->right, result);
}

static int find_ast(operationn op, int left, int right) {
	for(auto& e : bsdata<asti>()) {
		if(e.op == op && e.left == left && e.right == right)
			return &e - bsdata<asti>::begin();
	}
	return -1;
}

static int check_zero(int value, const char* format_error) {
	if(!value) {
		value = 1;
		error(format_error);
	}
	return value;
}

static int check_zero(int value) {
	return check_zero(value, "Division by zero");
}

int arifmetic(operationn op, int v1, int v2) {
	switch(op) {
	case Plus: return v1 + v2;
	case Minus: return v1 - v2;
	case Mul: return v1 * v2;
	case Div: return v1 / check_zero(v2);
	case DivRest: return v1 % check_zero(v2);
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

static asti ast_object(int i) {
	if(i == -1)
		return {Nop, -1, -1};
	return bsdata<asti>::get(i);
}

// Optimize operation before ut to syntax tree
static void optimize(operationn& op, int& left, int& right) {
	if(isterminal(op))
		return;
	else if(op >= Plus && op <= And) {
		asti p1 = ast_object(left);
		asti p2 = ast_object(right);
		if(p1.op == Number && p2.op == Number) {
			right = arifmetic(op, p1.right, p2.right);
			left = -1;
			op = Number;
		}
	}
}

int ast_add(operationn op, int left, int right) {
	optimize(op, left, right);
	auto sid = find_ast(op, left, right);
	if(sid == -1 && !isstrict(op))
		sid = find_ast(op, right, left);
	if(sid == -1) {
		auto p = bsdata<asti>::add();
		p->op = op;
		p->left = left;
		p->right = right;
		sid = p - bsdata<asti>::begin();
	}
	return sid;
}

static int ast_add(operationn op, int value) {
	return ast_add(op, -1, value);
}

static void parse_number() {
	auto p1 = p;
	last_number = 0;
	p = psnum(p, last_number);
	if(p1 != p)
		skipws();
}

static void literal() {
	stringbuilder sb(last_string); sb.clear();
	if(*p == '\"') {
		p = sb.psstr(p + 1, '\"');
		skipws();
	}
}

static void parse_identifier() {
	stringbuilder sb(last_string); sb.clear();
	if(!isidentifier())
		error("Expected identifier");
	while(*p && (ischa(*p) || isnum(*p) || *p == '_'))
		sb.add(*p++);
	skipws();
}

static void parse_flags() {
	last_flags = 0;
	while(*p) {
		if(word("static"))
			last_flags |= FG(Static);
		else
			break;
	}
}

static int create_section(int ids) {
	auto p = bsdata<sectioni>::add();
	p->ids = ids;
	p->data = 0;
	p->size = 0;
	return p - bsdata<sectioni>::begin();
}

static int create_define(int ids, int ast) {
	auto p = bsdata<definei>::add();
	p->ids = ids;
	p->ast = ast;
	return p - bsdata<definei>::begin();
}

static int create_define(int ids, int ast, const char* error_format) {
	auto sid = find_define(ids);
	if(sid != -1) {
		if(error_format)
			error(error_format, string_name(ids));
	} else
		sid = create_define(ids, ast);
	return sid;
}

int create_symbol(int id, int type, unsigned flags, int scope, int parent) {
	auto p = bsdata<symboli>::add();
	p->ids = id;
	if(scope == -1)
		scope = getscope();
	p->scope = scope;
	p->parent = parent;
	p->type = type;
	p->flags = flags;
	p->value = -1;
	p->frame = 0;
	p->count = 0;
	p->instance.sid = -1;
	p->instance.offset = 0;
	if(p->scope == PointerScope)
		p->size = size_of_pointer;
	else
		p->size = symbol_size(p->type);
	return p->getindex();
}

static int create_symbol(int ids, int type, unsigned flags, int scope, int parent, const char* error_format) {
	if(scope == -1)
		scope = getscope();
	auto sid = find_symbol(ids, scope, parent);
	if(sid != -1) {
		if(error_format)
			error(error_format, string_name(ids));
	} else
		sid = create_symbol(ids, type, flags, scope, parent);
	return sid;
}

static const char* find_module_url(const char* folder, const char* id, const char* ext) {
	static char temp[260]; stringbuilder sb(temp); sb.clear();
	if(!folder || !id || !ext)
		return 0;
	sb.add(folder);
	sb.add("/");
	sb.add(id);
	sb.change('.', '/');
	sb.add(".");
	sb.add(ext);
	if(!io::file::exist(temp))
		return 0;
	return temp;
}

static const char* get_type_url(const char* id) {
	auto p = find_module_url(project_url, id, "pcn");
	if(!p)
		p = find_module_url(library_url, id, "pcn");
	return p;
}

static void instance_symbol(int sid) {
	auto section = ModuleSection;
	if(symbol(sid, Static)) {
		if(symbol_ast(sid) == -1)
			section = UDataSection;
		else
			section = DataSection;
	} else if(current_scope)
		section = LocalSection;
	instance_symbol(sid, section);
}

static int create_module(int ids) {
	auto sid = find_symbol(ids, TypeScope, 0);
	if(sid == -1) {
		auto url = get_type_url(string_name(ids));
		if(!url) {
			error("Can't find module `%1`", string_name(ids));
			return -1;
		}
		auto push_module = module_sid;
		module_sid = create_symbol(ids, -1, 0, TypeScope, 0);
		symbol_type(module_sid, module_sid);
		calculator_file_parse(url);
		symbol(module_sid, Complete);
		sid = module_sid;
		module_sid = push_module;
	}
	return sid;
}

static int find_variable(int ids) {
	auto sid = -1;
	for(auto p = current_scope; sid == -1 && p; p = p->previous)
		sid = find_symbol(ids, p->scope, module_sid);
	if(sid == -1 && function_params != -1)
		sid = find_symbol(ids, 0, function_params);
	if(sid == -1)
		sid = find_symbol(ids, 0, module_sid);
	return sid;
}

int dereference(int type) {
	if(type == -1)
		return -1;
	auto p = bsdata<symboli>::begin() + type;
	return p->type;
}

int reference(int type) {
	if(type == -1)
		return -1;
	auto sid = find_symbol(type, PointerScope, 0);
	if(sid != -1)
		return sid;
	return create_symbol(type, type, 0, PointerScope, 0);
}

static void parse_type() {
	last_type = -1;
	last_flags = 0;
	if(!isidentifier())
		return;
	auto p_push = p;
	parse_flags();
	parse_identifier();
	auto ids = strings.add(last_string);
	last_type = find_symbol(ids, TypeScope, 0);
	if(last_type == -1) {
		auto sid = find_define(ids);
		if(sid != -1)
			find_type(define_ast(sid), last_type);
	}
	if(last_type == -1) {
		p = p_push;
		return;
	}
	last_type = symbol_type(last_type);
	while(match("*"))
		last_type = reference(last_type);
}

static bool parse_member_declaration() {
	auto push_p = p;
	parse_type();
	if(last_type == -1) {
		p = push_p;
		return false;
	}
	if(!isidentifier()) {
		p = push_p;
		return false;
	}
	return true;
}

static void parse_url_identifier() {
	stringbuilder sb(last_string); sb.clear();
	if(!isidentifier())
		error("Expected identifier");
	while(*p && (ischa(*p) || isnum(*p) || *p == '_' || *p == '.'))
		sb.add(*p++);
	skipws();
}

static void binary(int& a, operationn op, int b) {
	a = ast_add(op, a, b);
}

static void add_list(operationn op, int& result, int value) {
	if(value == -1)
		return;
	if(result == -1)
		result = value;
	else
		result = ast_add(op, result, value);
}

static void add_list(int& result, int value) {
	add_list(List, result, value);
}

static int parameter_list() {
	auto result = -1;
	while(true) {
		add_list(result, expression());
		if(match(","))
			continue;
		break;
	}
	return result;
}

static int unary(operationn op, int b) {
	return ast_add(op, b);
}

static int unary() {
	if(match("--"))
		return unary();
	else if(match("-"))
		return unary(Neg, unary());
	else if(match("++"))
		return unary();
	else if(match("+"))
		return unary();
	else if(match("!"))
		return unary(Not, unary());
	else if(match("*"))
		return unary(Dereference, unary());
	else if(match("&"))
		return unary(AdressOf, unary());
	else if(match("(")) {
		parse_type();
		if(last_type != -1 && match(")"))
			return ast_add(Cast, last_type, unary());
		auto a = expression();
		skip(")");
		return a;
	} else if(p[0] == '\"') {
		literal();
		return ast_add(Text, strings.add(last_string));
	} else if(p[0] >= '0' && p[0] <= '9') {
		parse_number();
		return ast_add(Number, last_number);
	} else if(isidentifier()) {
		parse_type();
		if(last_type != -1)
			return ast_add(Symbol, last_type);
		parse_identifier();
		auto ids = string_id(last_string);
		auto idf = find_define(ids);
		if(idf != -1)
			return define_ast(idf);
		auto sid = find_variable(ids);
		if(sid == -1) {
			error("Symbol `%1` not exist", string_name(ids));
			sid = create_symbol(ids, i32, 0, -1, module_sid);
		}
		return ast_add(Symbol, sid);
	} else {
		error("Expected unary expression.");
		return -1;
	}
}

static int postfix() {
	auto a = unary();
	while(*p) {
		if(match("(")) {
			auto params = -1;
			if(*p != ')')
				params = parameter_list();
			skip(")");
			binary(a, Call, params);
		} else if(match("[")) {
			auto b = expression();
			skip("]");
		} else if(match("++")) {

		} else if(match("--")) {

		} else if(match(".")) {
			parse_identifier();
			auto ids = string_id(last_string);
			binary(a, Point, ast_add(Identifier, ids));
		} else
			break;
	}
	return a;
}

static int multiplication() {
	auto a = postfix();
	while((p[0] == '*' || p[0] == '/' || p[0] == '%') && p[1] != '=') {
		char s = p[0]; skipws(1);
		operationn op;
		switch(s) {
		case '/': op = Div; break;
		case '%': op = DivRest; break;
		default: op = Mul; break;
		}
		binary(a, op, postfix());
	}
	return a;
}

static int addiction() {
	auto a = multiplication();
	while((p[0] == '+' || p[0] == '-') && p[1] != '=') {
		char s = p[0]; skipws(1);
		operationn op;
		switch(s) {
		case '+': op = Plus; break;
		case '-': op = Minus; break;
		default: op = Mul; break;
		}
		binary(a, op, multiplication());
	}
	return a;
}

static int binary_cond() {
	auto a = addiction();
	while((p[0] == '>' && p[1] != '>')
		|| (p[0] == '<' && p[1] != '<')
		|| (p[0] == '=' && p[1] == '=')
		|| (p[0] == '!' && p[1] == '=')) {
		char t1 = *p++;
		char t2 = 0;
		if(p[0] == '=')
			t2 = *p++;
		skipws();
		operationn op;
		switch(t1) {
		case '>':
			op = Greater;
			if(t2 == '=')
				op = GreaterEqual;
			break;
		case '<':
			op = Less;
			if(t2 == '=')
				op = LessEqual;
			break;
		case '!': op = NotEqual; break;
		default: op = Equal; break;
		}
		binary(a, op, addiction());
	}
	return a;
}

static int binary_and() {
	auto a = binary_cond();
	while(p[0] == '&' && p[1] != '&') {
		skipws(1);
		binary(a, BinaryAnd, binary_cond());
	}
	return a;
}

static int binary_xor() {
	auto a = binary_and();
	while(p[0] == '^') {
		skipws(1);
		binary(a, BinaryXor, binary_and());
	}
	return a;
}

static int binary_or() {
	auto a = binary_xor();
	while(p[0] == '|' && p[1] != '|') {
		skipws(1);
		binary(a, BinaryOr, binary_xor());
	}
	return a;
}

static int binary_shift() {
	auto a = binary_or();
	while((p[0] == '>' && p[1] == '>') || (p[0] == '<' && p[1] == '<')) {
		operationn op;
		switch(p[0]) {
		case '>': op = ShiftRight; break;
		default: op = ShiftLeft; break;
		}
		skipws(2);
		binary(a, op, binary_or());
	}
	return a;
}

static int logical_and() {
	auto a = binary_shift();
	while(match("and"))
		binary(a, And, binary_shift());
	return a;
}

static int logical_or() {
	auto a = logical_and();
	while(match("or"))
		binary(a, Or, logical_and());
	return a;
}

static int expression() {
	auto result = logical_or();
	while(match("if")) {
		auto a = logical_or();
		auto b = expression();
	}
	return result;
}

static int initialization_list() {
	return expression();
}

static int getmoduleposition() {
	return p - p_start;
}

static int assigment() {
	auto a = expression();
	while(p[0] == '=') {
		skipws(1);
		binary(a, Assign, expression());
	}
	return a;
}

static void parse_array_declaration(int sid) {
	if(match("[")) {
		auto index = const_number(expression());
		symbol_count(sid, index);
		skip("]");
	}
}

static int parse_local_declaration() {
	if(parse_member_declaration()) {
		auto type = last_type;
		auto flags = last_flags;
		parse_identifier();
		auto ids = strings.add(last_string);
		auto sid = create_symbol(ids, type, flags, -1, module_sid);
		parse_array_declaration(sid);
		if(match("="))
			symbol_ast(sid, initialization_list());
		instance_symbol(sid);
		return ast_add(Assign, sid, symbol_ast(sid));
	} else
		return assigment();
}

static bool islinefeed() {
	return *p == 10 || *p == 13;
}

static bool isend() {
	if(*p == 10 || *p == 13) {
		p = skipcr(p);
		return true;
	}
	return false;
}

static int statements();

static void error_encounter(const char* p1, const char* p2) {
	error("Encouter `%1` without `%2` statement", p1, p2);
}

static int statement() {
	if(match("pass"))
		return -1;
	else if(match("if")) {
		auto e = expression();
		auto s = statements();
		auto r = ast_add(If, e, s);
		while(match("elif")) {
			auto e = expression();
			auto s = statements();
			add_list(Else, r, ast_add(If, e, s));
		}
		if(match("else")) {
			auto s = statements();
			add_list(Default, r, ast_add(If, s));
		}
		return r;
	} else if(match("match")) {
		auto e = expression();
		auto r = ast_add(Switch, e);
		scopei push_scope(getmoduleposition());
		if(!next_statement()) {
			error("Expected statement");
			return -1;
		}
		while(match("case")) {
			auto e = expression();
			auto s = statements();
			add_list(Case, r, ast_add(If, e, s));
		}
		return r;
	} else if(match("while")) {
		auto e = expression();
		auto s = statements();
		return ast_add(While, e, s);
	} else if(match("elif")) {
		auto e = expression();
		auto s = statements();
		error_encounter("elif", "if");
		return -1;
	} else if(match("else")) {
		auto s = statements();
		error_encounter("else", "if");
		return -1;
	} else if(match("for")) {
		auto v = parse_local_declaration();
		skip("in");
		auto c = expression();
		auto s = statements();
		return ast_add(While, ast_add(In, v, c), s);
	} else if(match("return"))
		return ast_add(Return, expression());
	else
		return parse_local_declaration();
}

static int statements() {
	scopei push_scope(getmoduleposition());
	auto r = -1;
	while(next_statement())
		add_list(r, statement());
	return r;
}

static void parse_enum() {
	if(!word("enum"))
		return;
	auto index = 0;
	skip("{");
	while(*p) {
		parse_identifier();
		if(match("="))
			index = const_number(expression());
		auto ids = strings.add(last_string);
		auto sid = create_define(ids, ast_add(Number, index), "Identifier `%1` is already defined");
		if(!match(","))
			break;
	}
	skip("}");
}

static void parse_parameters() {
	auto scope = getscope();
	while(*p && *p != ')') {
		auto ast = -1;
		auto p1 = p;
		parse_type();
		if(p1 == p)
			break;
		auto type = last_type;
		auto flags = last_flags;
		parse_identifier();
		if(match("="))
			ast = expression();
		auto ids = strings.add(last_string);
		auto sid = create_symbol(ids, type, flags, scope, function_params, "Parameter `%1` is already defined");
		symbol_ast(sid, ast);
		if(match(","))
			continue;
	}
	skip(")");
}

static void parse_private(unsigned& f) {
	if(*p == '_') {
		f |= FG(Private);
		p++;
	}
}

static void parse_declaration() {
	if(!parse_member_declaration())
		return;
	auto type = last_type;
	auto flags = last_flags;
	while(isidentifier()) {
		auto member_flags = flags;
		parse_private(member_flags);
		parse_identifier();
		auto ids = strings.add(last_string);
		auto sid = create_symbol(ids, type, member_flags, -1, module_sid, "Module member `%1` is already defined");
		if(match("(")) {
			symbol_set(sid, Function);
			pushparam push(sid);
			parse_parameters();
			symbol_ast(sid, statements());
			symbol_frame(sid, scope_maximum);
			break;
		} else {
			parse_array_declaration(sid);
			if(match("="))
				symbol_ast(sid, initialization_list());
			instance_symbol(sid);
			if(match(","))
				continue;
			break;
		}
	}
}

static void parse_import() {
	if(!word("import"))
		return;
	parse_url_identifier();
	auto ids = strings.add(last_string);
	auto sid = create_module(ids);
	if(match("as")) {
		parse_identifier();
		auto als = string_id(last_string);
		create_define(als, ast_add(Symbol, sid));
	}
}

static void parse_define() {
	if(!word("define"))
		return;
	parse_identifier();
	skip("=");
	auto ids = string_id(last_string);
	create_define(ids, expression());
}

static void parse_module() {
	while(*p) {
		skip_line_feed();
		auto push_p = p;
		parse_import();
		if(p != push_p)
			continue;
		parse_define();
		if(p != push_p)
			continue;
		parse_enum();
		if(p != push_p)
			continue;
		parse_declaration();
		if(p != push_p)
			continue;
		if(!errors_count)
			error("Expected declaration");
		break;
	}
}

static void add_symbol(symboln v, const char* id, int size) {
	auto ids = strings.add(id);
	if(find_symbol(ids, TypeScope, 0) != -1)
		return;
	create_symbol(ids, -1, FG(Predefined), TypeScope, 0);
	bsdata<symboli>::get(v).size = size;
}

static void add_symbol(sectionn v, const char* id) {
	auto ids = strings.add(id);
	if(find_section(ids) != -1)
		return;
	create_section(ids);
}

static void symbol_initialize() {
	add_symbol(Void, "void", 0);
	add_symbol(Auto, "auto", 0);
	add_symbol(i8, "char", 1);
	add_symbol(u8, "uchar", 1);
	add_symbol(i16, "short", 2);
	add_symbol(u16, "ushort", 2);
	add_symbol(i32, "int", 4);
	add_symbol(u32, "uint", 4);
	add_symbol(i64, "long", 8);
	add_symbol(u64, "ulong", 8);
	add_symbol(ModuleSection, ".module");
	add_symbol(LocalSection, ".local");
	add_symbol(DataSection, ".data");
	add_symbol(UDataSection, ".bss");
}

static void calculator_initialize() {
	symbol_initialize();
	function_params = -1;
}

void calculator_parse(const char* code) {
	pushfile push(code);
	expression();
}

void calculator_file_parse(const char* url) {
	auto p1 = loadt(url);
	if(!p1) {
		error("Can't find file `%1`", url);
		return;
	}
	calculator_initialize();
	pushfile push(p1);
	p_url = url;
	skipws();
	parse_module();
	delete p1;
}

void project_compile(const char* url) {
	auto push_project = project_url;
	errors_count = 0;
	project_url = url;
	calculator_initialize();
	create_module(strings.add("main"));
	project_url = push_project;
}