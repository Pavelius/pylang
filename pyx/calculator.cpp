#include "bsdata.h"
#include "calculator.h"
#include "flagable.h"
#include "io_stream.h"
#include "scope.h"
#include "section.h"
#include "stringbuilder.h"
#include "stringa.h"

BSDATAD(asti)
BSDATAD(definei)
BSDATAD(symboli)

calculator_fnprint	calculator_error_proc;
static int			function_params;
static int			last_type;
static unsigned		last_flags;
static long			last_number;
static char			last_string[256 * 256];
static const char*	p;
static const char*	p_start;
static const char*	p_url;
static stringa		strings;
static int			operations[128];
static int*			operation;
static int			errors_count;

static int			module_sid;
static const char*	project_url;
const char*			library_url;

static int			size_of_pointer = 4;

static int parse_expression();
static int unary();
static int expression();

int symboli::getindex() const {
	return this - bsdata<symboli>::begin();
}

bool isterminal(operationn v) {
	switch(v) {
	case Number: case Text: case Identifier: return true;
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

int predefined_symbol_size(int type) {
	switch(type) {
	case i8: case u8: return 1;
	case i16: case u16: return 2;
	case i32: case u32: return 4;
	default: return 0;
	}
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
	if(!calculator_error_proc)
		return;
	XVA_FORMAT(format)
	calculator_error_proc(p_url, format, format_param, create_example());
}

int define_ast(int sid) {
	if(sid == -1)
		return -1;
	return bsdata<definei>::get(sid).ast;
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
	return bsdata<symboli>::get(sid).instance.size;
}

static int calculate_symbol_size(int sid) {
	auto type = symbol_type(sid);
	if(symbol_scope(type) == PointerScope)
		return size_of_pointer;
	auto result = predefined_symbol_size(type);
	if(!result) {
		// Calculate each member size
		for(auto& e : bsdata<symboli>()) {
			if(e.parent == sid && e.scope == 0 && !e.is(Static))
				result += e.instance.size;
		}
	}
	return result;
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

static void symbol_alloc(int sid, int data_sid) {
	if(sid == -1 || data_sid == -1)
		return;
	auto& e = bsdata<symboli>::get(sid);
	if(e.instance.sid != -1)
		return;
	e.instance.sid = data_sid;
	e.instance.size = calculate_symbol_size(sid);
	if(data_sid == LocalSection) {
		if(current_scope) {
			e.instance.offset = current_scope->getsize();
			current_scope->size += e.instance.size;
		}
	} else if(data_sid == ModuleSection) {
		auto& et = bsdata<symboli>::get(module_sid);
		e.instance.offset = et.instance.size;
		et.instance.size += e.instance.size;
	} else {
		auto& s = bsdata<sectioni>::get(data_sid);
		e.instance.offset = s.size;
		s.size += e.instance.size;
	}
}

const char* string_name(int sid) {
	if(sid == -1)
		return "";
	return strings.get(sid);
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

static void skip_indent() {
	int errors = 0;
	for(auto i = 0; i<scope_ident; i++) {
		if(*p==' ')
			p++;
		else
			errors++;
	}
	if(errors)
		error("Expected ident %1i spaces", scope_ident);
}

static void next_statement() {
	if(match(";"))
		skipws();
	else if(*p==10 || *p==13 || *p==0)
		p = skipcr(p);
	else
		error("Expected line feed");
	skip_indent();
}

static bool isidentifier() {
	return ischa(*p);
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

static int findast(operationn op, int left, int right) {
	for(auto& e : bsdata<asti>()) {
		if(e.op == op && e.left == left && e.right == right)
			return &e - bsdata<asti>::begin();
	}
	return -1;
}

int ast_add(operationn op, int left, int right) {
	auto sid = findast(op, left, right);
	if(sid == -1 && !isstrict(op))
		sid = findast(op, right, left);
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

static void add_op(int a) {
	operation[0] = a;
	operation++;
}

static int pop_op() {
	auto result = -1;
	if(operation > operations) {
		operation--;
		result = operation[0];
	}
	return result;
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
		else if(word("public"))
			last_flags |= FG(Public);
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

static int create_symbol(int id, int type, unsigned flags, int scope, int parent) {
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
	p->instance.sid = -1;
	p->instance.offset = 0;
	p->instance.size = 0;
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
	auto p = find_module_url(project_url, id, "py");
	if(!p)
		p = find_module_url(library_url, id, "py");
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
	symbol_alloc(sid, section);
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

static void unary_operation(operationn v) {
	if(operation <= operations) {
		error("Unary operations stack corupt");
		return;
	}
	operation[-1] = ast_add(v, operation[-1]);
}

static void binary_operation(operationn v) {
	if(operation <= operations + 1) {
		error("Binary operations stack corupt");
		return;
	}
	operation[-2] = ast_add(v, operation[-2], operation[-1]);
	operation--;
}

static void add_list(int& result, int value) {
	if(value == -1)
		return;
	if(result == -1)
		result = value;
	else
		result = ast_add(List, result, value);
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
	if(match("--")) {
		skipws(2);
		return unary();
	} else if(match("-")) {
		skipws(1);
		return unary(Neg, unary());
	} else if(p[0] == '+') {
		if(p[1] == '+') {
			skipws(2);
			return unary();
		} else {
			skipws(1);
			return unary();
		}
	} else if(p[0]=='!') {
		skipws(1);
		return unary(Not, unary());
	} else if(p[0]=='*') {
		skipws(1);
		return unary(Dereference, unary());
	} else if(p[0]=='&') {
		skipws(1);
		return unary(AdressOf, unary());
	} else if(p[0]=='(') {
		skipws(1);
		parse_type();
		if(last_type != -1 && *p == ')') {
			auto type = last_type;
			skipws(1);
			return ast_add(Cast, type, unary());
		} else {
			auto a = expression();
			skip(")");
			return a;
		}
	} else if(p[0]=='\"') {
		literal();
		return ast_add(Text, strings.add(last_string));
	} else if(p[0]>='0' && p[0]<='9') {
		parse_number();
		return ast_add(Number, last_number);
	} else if(isidentifier()) {
		parse_identifier();
		auto ids = strings.add(last_string);
		auto idf = find_define(ids);
		if(idf != -1)
			return define_ast(idf);
		auto sid = find_variable(ids);
		if(sid == -1) {
			error("Symbol `%1` not exist", string_name(ids));
			sid = create_symbol(ids, i32, 0, -1, module_sid);
		}
		return ast_add(Identifier, sid);
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
		} else if(match("[")) {
			auto b = expression();
			skip("]");
		} else if(match("++")) {

		} else if(match("--")) {

		} else if(match(".")) {
			parse_identifier();
			auto ids = strings.add(last_string);
			auto sid = find_variable(ids);
			if(sid == -1)
				sid = create_symbol(ids, i32, 0, -1, module_sid);
			add_op(ast_add(Identifier, sid));
			binary_operation(Point);
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
	while(p[0] == '&' && p[1] == '&') {
		skipws(2);
		binary(a, And, binary_shift());
	}
	return a;
}

static int logical_or() {
	auto a = logical_and();
	while(p[0] == '|' && p[1] == '|') {
		skipws(2);
		binary(a, Or, logical_and());
	}
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

static int parse_initialization_list() {
	if(match("{")) {
		auto result = -1;
		do {
			auto p1 = p;
			parse_initialization_list();
			if(p1 == p)
				break;
			add_list(result, pop_op());
		} while(match(","));
		add_op(ast_add(Initialization, result));
		skip("}");
	} else
		return expression();
}

static int initialization_list() {
	parse_initialization_list();
	return pop_op();
}

static int getmoduleposition() {
	return p - p_start;
}

static int parse_assigment() {
	auto a = unary();
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

static void symbol_instance(int sid, sectionn secid) {
	auto& e = bsdata<symboli>::get(sid);
	auto& s = section(secid);
	e.instance.offset = s.ptr();
}

static int parse_local_declaration() {
	if(parse_member_declaration()) {
		auto type = last_type;
		auto flags = last_flags;
		parse_identifier();
		auto ids = strings.add(last_string);
		auto sid = create_symbol(ids, type, flags, -1, module_sid);
		parse_array_declaration(sid);
		instance_symbol(sid);
		if(match("="))
			symbol_ast(sid, initialization_list());
		symbol_instance(sid, LocalSection);
		return ast_add(Assign, sid, symbol_ast(sid));
	} else
		return parse_assigment();
}

static bool islinefeed() {
	return *p==10 || *p==13;
}

static bool isend() {
	if(*p==10 || *p==13) {
		p = skipcr(p);
		return true;
	}
	return false;
}

static int statements();

static int parse_statement() {
	if(match("pass"))
		return -1;
	else if(match("if")) {
		auto e = expression();
		auto s = statements();
		return ast_add(If, e, s);
	} else if(match("while")) {
		auto e = expression();
		auto s = statements();
		return ast_add(While, e, s);
	} else if(match("for")) {
		skip("(");
		parse_local_declaration();
		auto bs = pop_op();
		auto e = expression();
		skip(";");
		auto es = expression();
		skip(")");
		parse_statement();
		auto s = pop_op();
		add_list(s, es);
		add_list(bs, ast_add(While, e, s));
		add_op(bs);
	} else if(match("return"))
		return ast_add(Return, expression());
	else
		return parse_local_declaration();
}

static int statements() {
	scopei push_scope(getmoduleposition());
	auto r = -1;
	while(isend())
		add_list(r, parse_statement());
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
	skip(";");
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

static void parse_declaration() {
	if(!parse_member_declaration())
		return;
	auto type = last_type;
	auto flags = last_flags;
	while(isidentifier()) {
		parse_identifier();
		auto ids = strings.add(last_string);
		auto sid = create_symbol(ids, type, flags, -1, module_sid, "Module member `%1` is already defined");
		if(match("(")) {
			symbol_set(sid, Function);
			auto push_params = function_params;
			auto push_scope_maximum = scope_maximum;
			scope_maximum = 0;
			function_params = sid;
			parse_parameters();
			parse_statement();
			symbol_ast(pop_op());
			symbol_frame(sid, scope_maximum);
			scope_maximum = push_scope_maximum;
			function_params = push_params;
			break;
		} else {
			parse_array_declaration(sid);
			if(!symbol(sid, Static))
				instance_symbol(sid);
			if(match("="))
				symbol_ast(sid, initialization_list());
			if(match(","))
				continue;
			skip(";");
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
		auto als = strings.add(last_string);
		create_define(als, ast_add(Identifier, sid));
	}
}

static void parse_define() {
	if(!word("define"))
		return;
	parse_identifier();
	skip("=");
	auto ids = strings.add(last_string);
	create_define(ids, expression());
}

static void parse_module() {
	while(*p) {
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

void calculator_parse(const char* code) {
	auto push_p = p;
	auto push_p_start = p_start;
	p = code;
	p_start = code;
	expression();
	p_start = push_p_start;
	p = push_p;
}

static void add_symbol(symboln v, const char* id) {
	auto ids = strings.add(id);
	if(find_symbol(ids, TypeScope, 0) != -1)
		return;
	create_symbol(ids, -1, FG(Predefined), TypeScope, 0);
}

static void add_symbol(sectionn v, const char* id) {
	auto ids = strings.add(id);
	if(find_section(ids) != -1)
		return;
	create_section(ids);
}

static void symbol_initialize() {
	add_symbol(Void, "void");
	add_symbol(i8, "char");
	add_symbol(u8, "uchar");
	add_symbol(i16, "short");
	add_symbol(u16, "ushort");
	add_symbol(i32, "int");
	add_symbol(u32, "unsigned");
	add_symbol(i64, "long");
	add_symbol(u64, "ulong");
	add_symbol(ModuleSection, ".module");
	add_symbol(LocalSection, ".local");
	add_symbol(DataSection, ".data");
	add_symbol(UDataSection, ".bss");
}

static void calculator_initialize() {
	symbol_initialize();
	operation = operations;
	function_params = -1;
}

void calculator_file_parse(const char* url) {
	auto p1 = loadt(url);
	if(!p1) {
		error("Can't find file `%1`", url);
		return;
	}
	calculator_initialize();
	auto push_p = p;
	auto push_p_start = p_start;
	auto push_p_url = p_url;
	p = p1; p_start = p1; p_url = url;
	skipws();
	parse_module();
	p_url = push_p_url;
	p_start = push_p_start;
	p = push_p;
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