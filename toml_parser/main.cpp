
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

void print_toml_node(TomlNodes* node)
{
}

int main(int argc, char** argv)
{
	int fd;
	long file_size;
	char *buffer;
	_sopen_s(&fd, ".\\test.toml", _O_RDONLY, _SH_DENYRW, _S_IREAD);
	if (fd == -1) {
		printf("failed to open file");
	}

	file_size = _filelength(fd);
	if (file_size == -1) {
		printf("failed to read file size");
	}

	buffer = (char*)malloc(file_size);
	_read(fd, buffer, file_size);
	
	TomlNodes nodes = parse_toml("blah", buffer);
	
	return 0;
}