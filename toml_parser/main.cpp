#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <float.h>

#include "toml_parser.h"

int main(int argc, char** argv)
{
	TomlNode* root = parse_toml("blah", ".\\test.toml");

	return 0;
}