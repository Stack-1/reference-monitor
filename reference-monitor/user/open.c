#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	FILE *fp = fopen("/home/stack1/Desktop/temp.txt", "r+");
	const char *buffer = "Ciao";
	char read_buffer[16];
	int ret;

	if (fp == NULL)
	{
		puts("Couldn't open file");
		exit(0);
	}
	else
	{

		ret = fwrite(buffer, 1, strlen(buffer), fp);
		if(ret == -1){
			perror("");
		}
		
		
		fread(read_buffer, 1, strlen(read_buffer), fp);
		printf("Char read is %s\n",read_buffer);
		puts("Done");
		fclose(fp);
	}
	return 0;
}
