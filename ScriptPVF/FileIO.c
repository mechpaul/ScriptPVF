#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "FileIO.h"

//Also see "CreateNewDirectory" and "WriteCarriageReturn" functions for other UNIX #ifdef's
#ifdef UNIX
#include <sys/mode.h>
#include <sys/stat.h>
#else
#include "windows.h"
#endif

#define BLOCKSIZE 0x80000

/* FILE READING */

long GetEOF(FILE* PatchFile)
{
	long remember;
	long eof;

	remember = ftell(PatchFile);
	fseek(PatchFile, 0, SEEK_END);
	eof = ftell(PatchFile);
	fseek(PatchFile, remember, SEEK_SET);

	return eof;
}

long GetEOFFile(char* fileName)
{
	FILE* genericFile;
	int eof;

	genericFile = safe_fopen(fileName, "rb");
	eof = GetEOF(genericFile);
	fclose(genericFile);

	return eof;
}

int rS32(FILE* fileObj)
{
	int readValue;

	fread(&readValue, 4, 1, fileObj);

	return readValue;
}

unsigned char* ReadTheFile(char* fileName)
{
	FILE* genericFile;
	int eof;
	unsigned char* wholeFile;

	genericFile = safe_fopen(fileName, "rb");
	eof = GetEOF(genericFile);
	wholeFile = malloc(eof + 1);
	fread(wholeFile, eof, 1, genericFile);
	fclose(genericFile);
	wholeFile[eof] = '\0';

	return wholeFile;
}

/* END FILE READING */

/* PLATFORM DEPENDENT */

//Possible Windows/UNIX clashing here
void CreateNewDirectory(char* fileName)
{
#ifdef UNIX
	mkdir(fileName, 0755);
#else
	CreateDirectory(fileName, NULL);
#endif
}

void CreateNewDirectoryIterate(char* dirName)
{
	char realDirName[300];
	unsigned int i;

	for(i = 0; dirName[i] != '\0'; i++)
	{
		if(dirName[i] == '/')
		{
			memcpy(realDirName, dirName, i+1);
			realDirName[i+1] = '\0';
			CreateDirectory(realDirName, NULL);
		}
	}
}

//Possible Windows/UNIX clashing here
void WriteCarriageReturn(FILE* fileName)
{
#ifdef UNIX
	fwrite("\n", 1, 1, fileName);
#else
	fwrite("\r\n", 2, 1, fileName);
#endif
}

/* END PLATFORM DEPENDENT */

/* SAFE WRAPPERS */

unsigned char* safe_malloc(int size)
{
	unsigned char* block;

	block = malloc(size);
	if(block == NULL)
	{
		printf("Out of memory!\n");
		exit(EXIT_FAILURE);
	}
	return block;
}

unsigned char* safe_realloc(unsigned char* buffer, int size)
{

	buffer = realloc(buffer, size);
	if(buffer == NULL)
	{
		printf("Out of memory!\n");
		exit(EXIT_FAILURE);
	}
	return buffer;
}

FILE* safe_fopen(char* fileName, char* openType)
{
	FILE* genericFile;

	genericFile = fopen(fileName, openType);
	if(genericFile == NULL)
	{
		printf("File %s DNE!\n", fileName);
		exit(EXIT_FAILURE);
	}
	return genericFile;
}

/* SAFE WRAPPERS END */