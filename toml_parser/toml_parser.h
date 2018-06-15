#ifndef __TOML_PARSER_H__
#define __TOML_PARSER_H__

/*
Requires: 
	stdlib.h (if not #define'ing your own TOML_MALLOC)
	string.h
	assert.h
	varargs.h
*/

#ifndef TOML_MALLOC
#define TOML_MALLOC(s) malloc(s)
#endif

#ifndef IS_SPACE
#define IS_SPACE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r' || (c) == '\v')
#endif
#ifndef IS_DIGIT
#define IS_DIGIT(c) ('0' <= (c) && (c) <= '9')
#endif
#ifndef IS_ALPHA
#define IS_ALPHA(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z'))
#endif
#ifndef IS_ALNUM
#define IS_ALNUM(c) (IS_DIGIT(c) || IS_ALPHA(c))
#endif
#ifndef TO_LOWER
#define TO_LOWER(c) (('a' <= (c) && (c) <= 'z') ? (c) : (c) + ('a' - 'A'))
#endif

const char* dup_str(const char* str, size_t len)
{
	char* dest = (char*)TOML_MALLOC(len + 1);
	char* ptr = dest;
	while (*str && len)
	{
		*ptr++ = *str++;
		len--;
	}
	*ptr = 0;
	return dest;
}

struct Parser {
	const char* stream;
	const char* line_start;
};

enum TokenKind {
	TOKEN_EOF,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_EQ,
	TOKEN_COLON,
	TOKEN_NAME,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_STR,
	TOKEN_DOT,
	TOKEN_COMMA,
};

const char* token_name(TokenKind kind)
{
	switch (kind)
	{
	case TOKEN_EOF: return "<EOF>";
	case TOKEN_LBRACKET: return "[";
	case TOKEN_RBRACKET: return "]";
	case TOKEN_LBRACE: return "{";
	case TOKEN_RBRACE: return "}";
	case TOKEN_EQ: return "=";
	case TOKEN_COLON: return ":";
	case TOKEN_NAME: return "<NAME>";
	case TOKEN_INT: return "<INT>";
	case TOKEN_FLOAT: return "<FLOAT>";
	case TOKEN_STR: return "<string>";
	case TOKEN_DOT: return ".";
	case TOKEN_COMMA: return ",";
	default: return "<UNKNOWN>";
	}
}

struct SrcPos {
	const char* name;
	size_t line;
};

struct Token {
	TokenKind kind;
	SrcPos pos;
	const char* start;
	const char* end;
	const char* name;
	unsigned long long int_val;
	double float_val;
	const char* str_val;
};

Parser parser;
Token token;

const char* token_info()
{
	return token_name(token.kind);
}

void error(SrcPos pos, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("%s(%zu): Error: ", pos.name, pos.line);
	vprintf(fmt, args);
	va_end(args);
    printf("\n");
}

#define error_here(...) error(token.pos, __VA_ARGS__)

unsigned char char_to_digit(unsigned char c)
{
    switch (c)
    {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
        default:
            return 0;
    }
};

void scan_float(int sign)
{
    char* buf = NULL;
	while (IS_DIGIT(*parser.stream) || *parser.stream == '_')
	{
        if (*parser.stream != '_')
        {
            sb_push(buf, *parser.stream);
        }
		parser.stream++;
	}
	if (*parser.stream == '.')
	{
        sb_push(buf, *parser.stream);
		parser.stream++;
	}
	while (IS_DIGIT(*parser.stream) || *parser.stream == '_')
	{
        if (*parser.stream != '_')
        {
            sb_push(buf, *parser.stream);
        }
		parser.stream++;
	}
	if (TO_LOWER(*parser.stream) == 'e')
	{
        sb_push(buf, *parser.stream);
		parser.stream++;
		if (*parser.stream == '+' || *parser.stream == '-')
		{
            sb_push(buf, *parser.stream);
			parser.stream++;
		}
		if (!IS_DIGIT(*parser.stream))
		{
			error_here("Expected digit after float literal exponent, found '%c'.", *parser.stream);
		}
		while (IS_DIGIT(*parser.stream) || *parser.stream == '_')
		{
            if (*parser.stream != '_')
            {
                sb_push(buf, *parser.stream);
            }
			parser.stream++;
		}
	}
    sb_push(buf, 0);
	double val = strtod(buf, NULL);
    sb_free(buf);
	if (val == DBL_MAX)
	{
		error_here("Float literal overflow");
	}
	token.kind = TOKEN_FLOAT;
	token.float_val = val * sign;
}

void scan_int(int sign)
{
	int base = 10;
	const char* start_digits = parser.stream;
	unsigned long long val = 0;
	for (;;)
	{
        if (*parser.stream == '_')
        {
            parser.stream++;
            continue;
        }
		int digit = char_to_digit((unsigned char)*parser.stream);
		if (digit == 0 && *parser.stream != '0')
		{
			break;
		}
		if (digit >= base)
		{
			error_here("Digit '%c' out of range for base %d", *parser.stream, base);
			digit = 0;
		}
		if (val > (ULLONG_MAX - digit) / base)
		{
			error_here("Integer literal overflow");
			while (IS_DIGIT(*parser.stream))
			{
				parser.stream++;
			}
			val = 0;
		}
		val = val * base + digit;
		parser.stream++;
	}
	if (parser.stream == start_digits)
	{
		error_here("Expected base %d digit, got '%c'", base, *parser.stream);
	}
	token.kind = TOKEN_INT;
	token.int_val = val * sign;
}

char escape_to_char(unsigned char c)
{
	switch (c)
	{
	case '0': return '\0';
	case '\'': return '\'';
	case '"': return '"';
	case '\\': return '\\';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'v': return '\v';
	case 'b': return '\b';
	case 'a': return '\a';
	default: return c;
	}	
};

int scan_hex_escape(void) {
	assert(*parser.stream == 'x');
	parser.stream++;
	int val = char_to_digit((unsigned char)*parser.stream);
	if (!val && *parser.stream != '0') {
		error_here("\\x needs at least 1 hex digit");
	}
	parser.stream++;
	int digit = char_to_digit((unsigned char)*parser.stream);
	if (digit || *parser.stream == '0') {
		val *= 16;
		val += digit;
		if (val > 0xFF) {
			error_here("\\x argument out of range");
			val = 0xFF;
		}
		parser.stream++;
	}
	return val;
}

void scan_str(void) {
	assert(*parser.stream == '"');
	parser.stream++;
	char *str = NULL;
	if (parser.stream[0] == '"' && parser.stream[1] == '"') {
		parser.stream += 2;
		while (*parser.stream) {
			if (parser.stream[0] == '"' && parser.stream[1] == '"' && parser.stream[2] == '"') {
				parser.stream += 3;
				break;
			}
			if (*parser.stream != '\r') {
				// TODO: Should probably just read files in text mode instead.
				sb_push(str, *parser.stream);
			}
			if (*parser.stream == '\n') {
				token.pos.line++;
			}
			parser.stream++;
		}
		if (!*parser.stream) {
			error_here("Unexpected end of file within multi-line string literal");
		}
	}
	else {
		while (*parser.stream && *parser.stream != '"') {
			char val = *parser.stream;
			if (val == '\n') {
				error_here("String literal cannot contain newline");
				break;
			}
			else if (val == '\\') {
				parser.stream++;
				if (*parser.stream == 'x') {
					val = scan_hex_escape();
				}
				else {
					val = escape_to_char((unsigned char)*parser.stream);
					if (val == 0 && *parser.stream != '0') {
						error_here("Invalid string literal escape '\\%c'", *parser.stream);
					}
					parser.stream++;
				}
			}
			else {
				parser.stream++;
			}
			sb_push(str, val);
		}
		if (*parser.stream) {
			parser.stream++;
		}
		else {
			error_here("Unexpected end of file within string literal");
		}
	}
	sb_push(str, 0);
	token.kind = TOKEN_STR;
	token.str_val = str;
}

void next_token()
{
repeat:
	token.start = parser.stream;
	switch (*parser.stream)
	{
	case ' ': case '\n': case '\t': case '\v': case '\r':
		while (IS_SPACE(*parser.stream))
		{
			if (*parser.stream == '\n')
			{
				parser.line_start = parser.stream;
				token.pos.line++;
			}
			parser.stream++;
		}
		goto repeat;
	case '#':
		while (*parser.stream != '\n' && *parser.stream != 0)
		{
			parser.stream++;
		}
		goto repeat;
	case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    case '-': case '+': {
        int sign = 1;
        if (*parser.stream == '-')
        {
            parser.stream++;
            sign = -1;
        }
        if (*parser.stream == '+')
        {
            parser.stream++;
        }
        if (!IS_DIGIT(*parser.stream))
        {
            error_here("Expected digit after sign");
        }
		const char* start = parser.stream;
		while (IS_DIGIT(*parser.stream) || *parser.stream == '_')
		{
			parser.stream++;
		}
		char c = *parser.stream;
		parser.stream = start;
		if (c == '.' || TO_LOWER(c) == 'e')
		{
			scan_float(sign);
		}
		else
		{
			scan_int(sign);
		}
	} break;
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case '_': {
		while (IS_ALNUM(*parser.stream) || *parser.stream == '_' || *parser.stream == '.')
		{
			parser.stream++;
		}
		token.name = dup_str(token.start, parser.stream - token.start);
		token.kind = TOKEN_NAME;
	} break;
	case '[':
		token.kind = TOKEN_LBRACKET;
		parser.stream++;
		break;
	case ']':
		token.kind = TOKEN_RBRACKET;
		parser.stream++;
		break;
	case '{':
		token.kind = TOKEN_LBRACE;
		parser.stream++;
		break;
	case '}':
		token.kind = TOKEN_RBRACE;
		parser.stream++;
		break;
	case '.':
		token.kind = TOKEN_DOT;
		parser.stream++;
		break;
	case ',':
		token.kind = TOKEN_COMMA;
		parser.stream++;
		break;
	case '=':
		token.kind = TOKEN_EQ;
		parser.stream++;
		break;
	case 0:
		token.kind = TOKEN_EOF;
		parser.stream++;
		break;
	case '"':
		scan_str();
		break;
	default:
		assert(0);
		break;
	}
	token.end = parser.stream;
}

bool is_token(TokenKind kind)
{
	return token.kind == kind;
}

bool match_token(TokenKind kind)
{
	if (token.kind == kind)
	{
		next_token();
		return true;
	}
	else
	{
		return false;
	}
}

bool expect_token(TokenKind kind)
{
	if (token.kind == kind)
	{
		next_token();
		return true;
	}
	else
	{
		error_here("Expected token %s, got %s", token_name(kind), token_info());
		return false;
	}
}

/*
Syntax looks something like:

array = '[' value (, value)* ']'

value = INT
| FLOAT
| STRING
| array

stmt = NAME '=' value

table = '[' NAME ']' (stmt)+

list_item = '[' '[' NAME ']' ']' (stmt)+

list = (list_item)+
*/

enum TomlValueKind {
	TOMLVALUE_NONE,
    TOMLVALUE_BOOL,
	TOMLVALUE_INT,
	TOMLVALUE_FLOAT,
	TOMLVALUE_STR,
	TOMLVALUE_ARRAY,
    TOMLVALUE_INLINETABLE,
};

struct TomlNodes;

struct TomlValue {
	TomlValueKind kind;
	union {
        bool bool_val;
		long long int_val;
		double float_val;
		const char* str_val;
		struct {
			TomlValue** array_vals;
			size_t num_array_vals;
		};
        struct {
            TomlNodes* table_nodes;
        };
	};
};

struct TomlStmt {
	const char* name;
	TomlValue* value;
};

struct TomlTable {
	const char* name;
	TomlStmt** stmts;
	size_t num_stmts;
};

struct TomlList {
	const char* name;
	TomlStmt** stmts;
	size_t num_stmts;
};

enum TomlDeclKind {
	TOMLDECL_NONE,
	TOMLDECL_STMT,
	TOMLDECL_TABLE,
	TOMLDECL_LIST,
};

struct TomlNode {
	TomlDeclKind kind;
	union {
		TomlStmt* stmt;
		TomlTable* tbl;
		TomlList* list;
	};
};

struct TomlNodes {
	TomlNode** nodes;
	size_t num_nodes;
};

void* toml_dup(const void* src, size_t size)
{
	if (size == 0)
	{
		return NULL;
	}
	void* ptr = TOML_MALLOC(size);
	memcpy(ptr, src, size);
	return ptr;
}

#define TOML_DUP(x) toml_dup(x, num_##x * sizeof(*x))

TomlNodes* new_tomlnodes(TomlNode** nodes, size_t num_nodes)
{
	TomlNodes* result = (TomlNodes*)TOML_MALLOC(sizeof(TomlNodes));
	result->nodes = (TomlNode**)TOML_DUP(nodes);
	result->num_nodes = num_nodes;
	return result;
}

TomlTable* new_toml_table(const char* name, TomlStmt** stmts, size_t num_stmts)
{
	TomlTable* result = (TomlTable*)TOML_MALLOC(sizeof(TomlTable));
	result->name = name;
	result->stmts = (TomlStmt**)TOML_DUP(stmts);
	result->num_stmts = num_stmts;
	return result;
}

TomlList* new_toml_list(const char* name, TomlStmt** stmts, size_t num_stmts)
{
	TomlList* result = (TomlList*)TOML_MALLOC(sizeof(TomlList));
	result->name = name;
	result->stmts = (TomlStmt**)TOML_DUP(stmts);
	result->num_stmts = num_stmts;
	return result;
}

TomlStmt* new_toml_stmt(const char* name, TomlValue* value)
{
	TomlStmt* result = (TomlStmt*)TOML_MALLOC(sizeof(TomlStmt));
	result->name = name;
	result->value = value;
	return result;
}

TomlNode* parse_toml_stmt();

TomlValue* parse_toml_value()
{
	TomlValue* result = (TomlValue*)TOML_MALLOC(sizeof(TomlValue));
    if (is_token(TOKEN_NAME))
    {
        if (strcmp(token.name, "true") == 0)
        {
            result->kind = TOMLVALUE_BOOL;
            result->bool_val = true;
        }
        else if (strcmp(token.name, "false") == 0)
        {
            result->kind = TOMLVALUE_BOOL;
            result->bool_val = false;
        }
        else
        {
            error_here("Expected value type, found name");
        }
    }
	else if (is_token(TOKEN_INT))
	{
		result->kind = TOMLVALUE_INT;
		result->int_val = token.int_val;
	}
	else if (is_token(TOKEN_FLOAT))
	{
		result->kind = TOMLVALUE_FLOAT;
		result->float_val = token.float_val;
	}
	else if (is_token(TOKEN_STR))
	{
		result->kind = TOMLVALUE_STR;
		result->str_val = token.str_val;
	}
	else if (is_token(TOKEN_LBRACKET))
	{
		result->kind = TOMLVALUE_ARRAY;
		next_token();
		TomlValue** values = NULL;
		sb_push(values, parse_toml_value());
		while (is_token(TOKEN_COMMA))
		{
            next_token();
			sb_push(values, parse_toml_value());
		}
		expect_token(TOKEN_RBRACKET);
		size_t num_values = sb_count(values);
		result->array_vals = (TomlValue**)TOML_DUP(values);
		result->num_array_vals = num_values;
        sb_free(values);
	}
    else if (is_token(TOKEN_LBRACE))
    {
        result->kind = TOMLVALUE_INLINETABLE;
        next_token();
        TomlNode** stmts = NULL;
        sb_push(stmts, parse_toml_stmt());
        while (is_token(TOKEN_COMMA))
        {
            next_token();
            sb_push(stmts, parse_toml_stmt());
        }
        expect_token(TOKEN_RBRACE);
        size_t num_stmts = sb_count(stmts);
        result->table_nodes = new_tomlnodes(stmts, num_stmts);
        sb_free(stmts);
        return result;
    }
    else
    {
        error_here("Unexpected token %s", token_info());
    }

	next_token();
	return result;
}

TomlNode* parse_toml_stmt()
{
	const char* name = token.name;
	expect_token(TOKEN_NAME);
	expect_token(TOKEN_EQ);
	TomlValue* value = parse_toml_value();
	TomlNode* node = (TomlNode*)TOML_MALLOC(sizeof(TomlNode));
	node->kind = TOMLDECL_STMT;
	node->stmt = new_toml_stmt(name, value);
	return node;
}

TomlNode* parse_toml_list_item()
{
	expect_token(TOKEN_LBRACKET);
	const char* name = token.name;
	expect_token(TOKEN_NAME);
	TomlStmt** stmts = NULL;
	while (is_token(TOKEN_NAME))
	{
		TomlNode* stmt_node = parse_toml_stmt();
		sb_push(stmts, stmt_node->stmt);
	}
	TomlNode* node = (TomlNode*)TOML_MALLOC(sizeof(TomlNode));
	node->kind = TOMLDECL_LIST;
	node->list = new_toml_list(name, stmts, sb_count(stmts));
    sb_free(stmts);
	return node;
}

TomlNode* parse_toml_collection()
{
	expect_token(TOKEN_LBRACKET);
	if (match_token(TOKEN_LBRACKET))
	{
		return parse_toml_list_item();
	}
	else
	{
		const char* name = token.name;
		expect_token(TOKEN_NAME);
		expect_token(TOKEN_RBRACKET);
		TomlStmt** stmts = NULL;
		while (is_token(TOKEN_NAME))
		{
			TomlNode* stmt_node = parse_toml_stmt();
			sb_push(stmts, stmt_node->stmt);
		}
		TomlNode* node = (TomlNode*)TOML_MALLOC(sizeof(TomlNode));
		node->kind = TOMLDECL_TABLE;
		node->tbl = new_toml_table(name, stmts, sb_count(stmts));
        sb_free(stmts);
		return node;
	}
}

TomlNode* parse_node()
{
	if (is_token(TOKEN_LBRACKET))
	{
		return parse_toml_collection();
	}
	else if (is_token(TOKEN_NAME))
	{
		return parse_toml_stmt();
	}
	else
	{
		error_here("Expected one or more declarations");
		return NULL;
	}
}

TomlNodes* parse_toml(const char* name, const char* buf)
{
	parser.stream = buf;
	parser.line_start = parser.stream;
	token.pos.name = name;
	token.pos.line = 1;
	next_token();

	TomlNode** nodes = NULL;
	while (!is_token(TOKEN_EOF))
	{
        TomlNode* node = parse_node();
        if (!node)
        {
            assert(0);
            return NULL;
        }
		sb_push(nodes, node);
	}
	TomlNodes* result = (TomlNodes*)TOML_MALLOC(sizeof(TomlNodes));
	size_t num_nodes = sb_count(nodes);
	result->nodes = (TomlNode**)TOML_DUP(nodes);
	result->num_nodes = num_nodes;
    sb_free(nodes);
	return result;
}

#undef IS_DIGIT
#undef TOML_MALLOC

#endif
