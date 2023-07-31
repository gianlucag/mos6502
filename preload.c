#include <stdio.h>

#if defined(__linux__)
#include <ncurses.h>
#elif defined(_WIN32)
#include <conio.h>
#endif

int main(int argc, char *argv[])
{
	// check file existence
	for (int i = 1; i < argc; i++)
	{
		FILE* f = fopen(argv[i], "r");
		if (f == NULL)
		{
			printf("file not found: %s\n", argv[i]);
			return -1;
		}
		fclose(f);
	}

	// start the emualtor
	FILE* child = popen(argv[1], "w");
	if (child == NULL)
	{
		printf("failed to start the emulator\n");
		return -1;
	}

	// print the contents of the files in order
	for (int i = 2; i < argc; i++)
	{
		FILE* f = fopen(argv[i], "rb");
		int ch;
		while ((ch = fgetc(f)) != EOF)
		{
			fputc(ch, child);
		}
		fclose(f);
		fputc('\n', child); // separate files
	}

	// continue to read from stdin
	while (1)
	{
		int ch = getch();
		fputc(ch, child);
		if (ch == 3) // ASCII(3) = Ctrl+C
			break;
	}
	fclose(child);
}
