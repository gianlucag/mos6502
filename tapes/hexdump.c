#include <stdio.h>

int main(int argc, char* argv[])
{
	FILE* f = fopen(argv[1], "rb");
	if (argc != 2)
	{
		printf("usage: hexdump <file>\n");
		return -1;
	}
	if (f == NULL)
	{
		printf("no such file %s\n", argv[1]);
		return -1;
	}
	while (!feof(f))
	{
		printf(":");
		int ch;
		for(int i = 0; i < 8 && (ch = fgetc(f)) != EOF; i++)
		{
			printf(" %02X", (unsigned)ch);
		}
		printf("\n");
	}
}
