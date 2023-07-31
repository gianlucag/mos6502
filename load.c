#include <stdio.h>
#include <stdlib.h>

#if defined(__linux__)
#elif defined(_WIN32)
#include <windows.h>
#endif

#define PIPE_NAME "\\\\.\\pipe\\apple1"
static HANDLE pipe;
void init_pipe()
{
	pipe = CreateFile( 
         PIPE_NAME,   // pipe name 
         GENERIC_READ |  // read and write access 
         GENERIC_WRITE, 
         0,              // no sharing 
         NULL,           // default security attributes
         OPEN_EXISTING,  // opens existing pipe 
         0,              // default attributes 
         NULL);  
	if (pipe == INVALID_HANDLE_VALUE) 
    {
		printf("failed to connect to the emulator\n");
		printf("%d\n", GetLastError());
		exit(-1);
	}	
}

void end_pipe()
{
	CloseHandle(pipe);
}

void pipeout(char ch)
{
	DWORD num; BOOL succ;
	succ = WriteFile(pipe, &ch, 1, &num, NULL);
	if (!succ)
	{
		printf("failed to write to pipe\n");
		end_pipe();
		exit(-1);
	}
}

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

	init_pipe();

	// print the contents of the files in order
	for (int i = 1; i < argc; i++)
	{
		FILE* f = fopen(argv[i], "rb");
		int ch;
		while ((ch = fgetc(f)) != EOF)
		{
			pipeout(ch);
		}
		fclose(f);
		pipeout('\n'); // separate files
	}

	end_pipe();
}
