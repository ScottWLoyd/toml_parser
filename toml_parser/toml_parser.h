#ifndef __TOML_PARSER_H__
#define __TOML_PARSER_H__

/*
requires: 
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
#define TO_LOWER(c) (('a' <= (c) && (c) <= 'z') ? (c) + ('a' - 'A') : (c))
#endif

const char* dup_str(const char* str, size_t len)
{
	char* dest = (char*)TOML_MALLOC(len);
	while (*str)
	{
		*dest++ = *str++;
	}
	return dest;
}

enum TomlNodeKind {
	TOMLNODE_KEYVAL,
	TOMLNODE_TABLE,
	TOMLNODE_ARRAY,
};

struct TomlNode;

union TomlValueType {
	struct {
		bool negative;
		unsigned long long int_val;
	};
	double float_val;
	const char* str_val;
	struct {
		TomlNode** nodes;
		size_t num_nodes;
	};
};

struct TomlNode {
	TomlNodeKind kind;
	const char* name;
	TomlValueType value;
};

struct Parser {
	const char* stream;
	const char* line_start;

};

enum TokenKind {
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,
	TOKEN_LBRACE,
	TOKEN_RBRACE,
	TOKEN_EQ,
	TOKEN_COLON,
	TOKEN_NAME,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_DOT,
};

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
};

Parser parser;
Token token;

void error(SrcPos pos, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("%s(%zu): Error: ", pos.name, pos.line);
	vprintf(fmt, args);
	va_end(args);
}

#define error_here(...) error(token.pos, __VA_ARGS__)

void scan_float(void)
{
	const char* start = parser.stream;
	while (IS_DIGIT(*parser.stream))
	{
		parser.stream++;
	}
	if (*parser.stream == '.')
	{
		parser.stream++;
	}
	while (IS_DIGIT(*parser.stream))
	{
		parser.stream++;
	}
	if (TO_LOWER(*parser.stream) == 'e')
	{
		parser.stream++;
		if (*parser.stream == '+' || *parser.stream == '-')
		{
			parser.stream++;
		}
		if (!IS_DIGIT(*parser.stream))
		{
			error_here("Expected digit after float literal exponent, found '%c'.", *parser.stream);
		}
		while (IS_DIGIT(*parser.stream))
		{
			parser.stream++;
		}
	}
	const char* end = parser.stream;
	double val = strtod(start, NULL);
	if (val == DBL_MAX)
	{
		error_here("Float literal overflow");
	}
	token.kind = TOKEN_FLOAT;
	token.float_val = val;
}

void scan_int(void)
{
	int base = 10;
	const char* start_digits = parser.stream;
	unsigned long long val = 0;
	for (;;)
	{
		int digit = *parser.stream - '0';
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
	token.int_val = val;
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
	case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
		const char* start = parser.stream;
		while (IS_DIGIT(*parser.stream))
		{
			parser.stream++;
		}
		char c = *parser.stream;
		parser.stream = start;
		if (c == '.' || c == 'e')
		{
			scan_float();
		}
		else
		{
			scan_int();
		}
	} break;
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case '_': {
		while (IS_ALNUM(*parser.stream) || *parser.stream == '_')
		{
			parser.stream++;
		}
		token.name = dup_str(token.start, parser.stream - token.start);
		token.kind = TOKEN_NAME;
	} break;
	case '[':
		token.kind = TOKEN_LBRACKET;
		break;
	case ']':
		token.kind = TOKEN_RBRACKET;
		break;
	case '{':
		token.kind = TOKEN_LBRACE;
		break;
	case '}':
		token.kind = TOKEN_RBRACE;
		break;
	case '.':
		token.kind = TOKEN_DOT;
		break;
	case '=':
		token.kind = TOKEN_EQ;
		break;
	case '"':
		assert(0);
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

TomlNode* parse_node(void)
{
	if (match_token(TOKEN_LBRACKET))
	{
		if (match_token(TOKEN_LBRACKET))
		{
			if (is_token(TOKEN_NAME))
			{
				const char* name = token.name;
				next_token();
				TomlNode* node = (TomlNode*)TOML_MALLOC(sizeof(TomlNode));
				node->kind = TOMLNODE_ARRAY;
				node->name = name;
			}
			else
			{
				error_here("Expected array name");
			}
		}
	}
	else if (match_token(TOKEN_NAME))
	{

	}
	else
	{
		error_here("Expected a table or key/value pair");
	}
}

TomlNode* parse_toml(const char* name, const char* buf)
{
	parser.stream = buf;
	parser.line_start = parser.stream;
	token.pos.name = name;
	token.pos.line = 0;
	next_token();

	TomlNode* root = parse_node();
	return root;
}

#undef IS_DIGIT
#undef TOML_MALLOC

#endif
