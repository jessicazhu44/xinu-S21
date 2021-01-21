#include <xinu.h>
#include <stdio.h>
#include <string.h>

shellcmd xsh_hello(int nargs, char *args[])
{
	// char mystr[64];
	if (nargs > 2) {
		fprintf(stderr, "%s: too many arguments\n", args[0]);
		return 1;
	}
	printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);

	return 0;
}
