#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ModesOp.h"

#define CREATE "cache"
#define UNPACK "unpack"
#define DIFF "update"
#define DECRYPT "decrypt"
#define VERSION "1.33"

void PrintMenu(void);

int main(int argc, char* argv[])
{
	if(argc == 2)
	{
		if(strcmp(argv[1], CREATE) == 0)
		{
			Create();
		}
		else if(strcmp(argv[1], UNPACK) == 0)
		{
			Unpack();
		}
		else if(strcmp(argv[1], DIFF) == 0)
		{
			Diff();
		}
		else if(strcmp(argv[1], DECRYPT) == 0)
		{
			Decrypt();
		}
		else
		{
			PrintMenu();
		}
	}
	else
	{
		PrintMenu();
	}

	return 0;
}

void PrintMenu(void)
{
	printf("Version %s\n", VERSION);
	printf("Commands:\n\n");
	printf("ScriptPVF.exe %s\n", CREATE);
	printf("Create a cache file of Script.pvf\n\n");
	printf("ScriptPVF.exe %s\n", UNPACK);
	printf("Unpack the Script.pvf file and create a cache of it.\n\n");
	printf("ScriptPVF.exe %s\n", DIFF);
	printf("ScriptCache.cache MUST exist! This is a differential analyzer to see differences between patches.\n\n");
	printf("ScriptPVF.exe %s\n", DECRYPT);
	printf("Decrypt Script.pvf and output the decrypted contents as a ScriptOutput.pvf file.\n\n");
}