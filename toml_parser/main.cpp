
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
            printf("%zd", val->int_val);
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
	
	return 0;
}