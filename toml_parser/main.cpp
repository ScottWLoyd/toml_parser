
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <float.h>

#include <fcntl.h>  
#include <sys/stat.h>  
#include <io.h>

#define global static
#define local_persist static
#define intern static

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
#define TO_LOWER(c) (('A' <= (c) && (c) <= 'Z') ? (c) + ('a' - 'A') : (c))
#endif

intern void error(const char* buf)
{
	printf("Error: %s\n", buf);
	exit(1);
}

#include "stretchy_buffer.h"
#include "toml_parser.h"


void print_toml_node(TomlNode* node);

void print_toml_value(TomlValue* val)
{
    switch (val->kind)
    {
        case TOMLVALUE_BOOL:
            printf("%s", val->bool_val ? "true" : "false");
            break;
        case TOMLVALUE_INT:
            printf("%d", (int)val->float_val);
            break;
        case TOMLVALUE_FLOAT:
            printf("%f", val->float_val);
            break;
        case TOMLVALUE_STR:
            printf("\"%s\"", val->str_val);
            break;
        case TOMLVALUE_INLINETABLE:
            printf("{ ");
            for (size_t i = 0; i < val->table_nodes->num_nodes; i++)
            {
                TomlNode* inline_node = val->table_nodes->nodes[i];
                print_toml_node(inline_node);
                printf(", ");
            }
            printf(" }");
            break;
        case TOMLVALUE_ARRAY:
            printf("[ ");
            for (size_t i = 0; i < val->num_array_vals; i++)
            {
                TomlValue* array_val = val->array_vals[i];
                print_toml_value(array_val);
                printf(", ");
            }
            printf(" ]");
            break;
        default:
            assert(0);
    }
}

void print_toml_stmt(TomlStmt* stmt)
{
    printf("%s = ", stmt->name);
    TomlValue* val = stmt->value;
    print_toml_value(val);
    printf("\n");
}

void print_toml_list(TomlList* list)
{
    printf("[[%s]]\n", list->name);
    for (size_t i = 0; i < list->num_stmts; i++)
    {
        TomlStmt* stmt = list->stmts[i];
        print_toml_stmt(stmt);
    }
    printf("\n");
}

void print_toml_table(TomlTable* tbl)
{
    printf("[%s]\n", tbl->name);
    for (size_t i = 0; i < tbl->num_stmts; i++)
    {
        TomlStmt* stmt = tbl->stmts[i];
        print_toml_stmt(stmt);
    }
    printf("\n");
}

void print_toml_node(TomlNode* node)
{
    switch (node->kind)
    {
        case TOMLDECL_STMT:
            print_toml_stmt(node->stmt);
            break;
        case TOMLDECL_LIST:
            print_toml_list(node->list);
            break;
        case TOMLDECL_TABLE:
            print_toml_table(node->tbl);
            break;
        default:
            assert(0);
            break;
    }

}

int main(int argc, char** argv)
{
    char* buffer;
    FILE* file = fopen(".\\test.toml", "r");
    fseek(file, 0, SEEK_END);
    long end = ftell(file);
    fseek(file, 0, SEEK_SET);

    buffer = (char*)malloc(end);
    size_t len = fread(buffer, 1, end, file);
    buffer[len] = 0;
	
	TomlNodes* nodes = parse_toml("blah", buffer);

    for (int i = 0; i < nodes->num_nodes; i++)
    {
        TomlNode* node = nodes->nodes[i];
        print_toml_node(node);
    }
	
    TomlNodes* results = toml_find_nodes(nodes->nodes, nodes->num_nodes, "test");
    assert(results->num_nodes == 1);
    results = toml_find_nodes(nodes->nodes, nodes->num_nodes, "table");
    assert(results->num_nodes == 3);
    results = toml_find_nodes(nodes->nodes, nodes->num_nodes, "float");
    assert(results->num_nodes == 4);
    results = toml_find_nodes(nodes->nodes, nodes->num_nodes, "products");
    assert(results->num_nodes == 3);
    results = toml_find_nodes(nodes->nodes, nodes->num_nodes, "bool");
    assert(results->num_nodes == 0);
    results = toml_find_nodes(nodes->nodes, nodes->num_nodes, "boolean");
    assert(results->num_nodes == 1);
    results = toml_find_nodes(nodes->nodes, nodes->num_nodes, "integer.key1");
    assert(results->num_nodes == 1);
    results = toml_find_nodes(nodes->nodes, nodes->num_nodes, "products.name");
    assert(results->num_nodes == 2);

	return 0;
}