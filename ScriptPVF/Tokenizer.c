#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Tokenizer.h"
#include "FileIO.h"
#include "Checksum.h"

#define SCRIPT_PVF "ScriptPVF/"
#define UPDATED "Updated/"
#define ORIGINAL "orig"

/* BEGIN TOKENS */

struct fileHeader GetHeader(FILE* Script)
{
	struct fileHeader scriptHeader;
	int remember;

	remember = ftell(Script);
	fseek(Script, 0, SEEK_SET);

	//Verify the header isn't encrypted
	scriptHeader.lengthChecksum = GetIntFile(Script);
	if(scriptHeader.lengthChecksum != 0x24)
	{
		printf("This unpacker does not support this Script.pvf format.\n");
		exit(EXIT_FAILURE);
	}
	fseek(Script, 0, SEEK_SET);

	//Rest of header
	scriptHeader.checksum = GetStringFile(Script);
	scriptHeader.lengthChecksum = strlen(scriptHeader.checksum);
	scriptHeader.version = GetIntFile(Script);
	scriptHeader.lengthDirTree = GetIntFile(Script);
	scriptHeader.dirTreeChecksum = GetIntFile(Script);
	scriptHeader.numFiles = GetIntFile(Script);

	//Convenience variables
	scriptHeader.eof = GetEOF(Script);
	scriptHeader.dirTreeStart = ftell(Script);
	scriptHeader.fileTreeStart = scriptHeader.dirTreeStart + scriptHeader.lengthDirTree;
	scriptHeader.lengthFileTree = scriptHeader.eof - scriptHeader.fileTreeStart;

	fseek(Script, remember, SEEK_SET);

	return scriptHeader;
}

unsigned char* GetString(unsigned char** block)
{
	int lengthString;
	unsigned char* thisString;

	lengthString = GetInt(block);
	thisString = safe_malloc(lengthString + 1);
	memcpy(thisString, *block, lengthString);
	thisString[lengthString] = '\0';
	*block += lengthString;

	return thisString;
}

unsigned char* GetStringFile(FILE* Script)
{
	int lengthString;
	unsigned char* thisString;

	lengthString = GetIntFile(Script);
	thisString = safe_malloc(lengthString + 1);
	fread(thisString, lengthString, 1, Script);
	thisString[lengthString] = '\0';

	return thisString;
}

unsigned int GetInt(unsigned char** fileBlock)
{
	unsigned int eachInt;

	eachInt = *((unsigned int*) *fileBlock);
	*fileBlock += 4;
	return eachInt;
}

unsigned int GetIntFile(FILE* Script)
{
	unsigned int thisInt;

	fread(&thisInt, 4, 1, Script);

	return thisInt;
}

struct eachFile GetDirTree(unsigned char** dirTree)
{
	struct eachFile scriptFile;

	scriptFile.fileNumber = GetInt(dirTree);
	scriptFile.fileName = GetString(dirTree);
	scriptFile.lengthString = strlen(scriptFile.fileName);
	scriptFile.fileLength = GetInt(dirTree);
	scriptFile.checksum = GetInt(dirTree);
	scriptFile.relOffset = GetInt(dirTree);

	//Figuring out computedFileLength is for decryption purposes.
	//The decryption works in 32-bit blocks, so it's necessary for the
	//file read from Script.pvf to be in multiples of 32-bits. When the file is written
	//from memory to the hard drive, the fileLength attribute is used to determine the real length
	//of the file.
	if(scriptFile.fileLength % 4 > 0)
	{
		scriptFile.computedFileLength = ((scriptFile.fileLength / 4) + 1) * 4;
	}
	else
	{
		scriptFile.computedFileLength = scriptFile.fileLength;
	}

	return scriptFile;
}

unsigned char* GetFileTree(unsigned char* internalFiles, struct eachFile scriptFile)
{
	unsigned char* eachFile;

	eachFile = safe_malloc(scriptFile.computedFileLength);
	memcpy(eachFile, internalFiles + scriptFile.relOffset, scriptFile.computedFileLength);

	return eachFile;
}

/* END TOKENS */

/* BEGIN DECRYPTION */

unsigned char* DecryptDirTree(FILE* Script, struct fileHeader scriptHeader)
{
	unsigned char* dirTree;

	fseek(Script, scriptHeader.dirTreeStart, SEEK_SET);

	dirTree = safe_malloc(scriptHeader.lengthDirTree);
	fread(dirTree, scriptHeader.lengthDirTree, 1, Script);
	DecryptBlock(dirTree, scriptHeader.lengthDirTree, scriptHeader.dirTreeChecksum);

	return dirTree;
}

void DecryptBlock(unsigned char* fileBlock, int lengthOfBlock, unsigned int decryptionKey)
{
	int each32bit;
	int i;

	for(i = 0; i < lengthOfBlock; i += 4)
	{
		//Decryption procedure

		// mov  eax, [ecx]
		each32bit = *((unsigned int*) fileBlock);

		// xor  eax, ebx
		each32bit = each32bit ^ decryptionKey;

		// mov  edx, eax
		// shl  edx, 1Ah
		// shr  eax, 6
		// or   eax, edx
		each32bit = _rotl(each32bit, 0x1A);

		// mov  [ecx], eax
		memcpy(fileBlock, &each32bit, 4);

		// add  ecx, 4
		fileBlock += 4;
	}
}

unsigned char* DecryptFileTree(FILE* Script, unsigned char* dirTree, struct fileHeader scriptHeader)
{
	unsigned char* internalFiles;
	unsigned char* eachFile;
	struct eachFile scriptFile;
	int i;

	fseek(Script, scriptHeader.fileTreeStart, SEEK_SET);
	internalFiles = safe_malloc(scriptHeader.lengthFileTree);
	fread(internalFiles, scriptHeader.lengthFileTree, 1, Script);

	for(i = 0; i < scriptHeader.numFiles; i++)
	{
		scriptFile = GetDirTree(&dirTree);

		//Decrypt file
		eachFile = GetFileTree(internalFiles, scriptFile);
		DecryptBlock(eachFile, scriptFile.computedFileLength, scriptFile.checksum);
		memcpy(internalFiles + scriptFile.relOffset, eachFile, scriptFile.computedFileLength);
		free(eachFile);
		free(scriptFile.fileName);
	}
	dirTree -= scriptHeader.lengthDirTree;
	return internalFiles;
}

/* END DECRYPTION */

/* BEGIN PROCEDURES */

void CreateCache(FILE* Cache, unsigned char* dirTree, unsigned char* internalFiles, struct fileHeader scriptHeader)
{
	int i;
	char nameOfFile[200];
	unsigned int checksum;
	unsigned char* eachFile;
	struct eachFile scriptFile;
	int stringLength;

	checksum = CalculateChecksumFile("Script.pvf");

	Cache = safe_fopen("ScriptCache.cache", "wb");
	fwrite(&checksum, 4, 1, Cache);
	fwrite(&scriptHeader.numFiles, 4, 1, Cache);

	for(i = 0; i < scriptHeader.numFiles; i++)
	{
		scriptFile = GetDirTree(&dirTree);

		//Fetch the string, prepending it with SCRIPT_PVF
		memcpy(nameOfFile, SCRIPT_PVF, strlen(SCRIPT_PVF));
		memcpy(nameOfFile + strlen(SCRIPT_PVF), scriptFile.fileName, scriptFile.lengthString);
		nameOfFile[strlen(SCRIPT_PVF) + scriptFile.lengthString] = '\0';

		//Calculate Checksum
		eachFile = GetFileTree(internalFiles, scriptFile);
		checksum = CalculateChecksum(eachFile, scriptFile.computedFileLength, 0);
		free(eachFile);

		//Commit
		stringLength = strlen(nameOfFile);
		fwrite(&stringLength, 4, 1, Cache);
		fwrite(nameOfFile, stringLength, 1, Cache);
		fwrite(&checksum, 4, 1, Cache);
	}
	dirTree -= scriptHeader.lengthDirTree;
}

void Output(unsigned char* dirTree, unsigned char* internalFiles, struct fileHeader scriptHeader)
{
	int i;
	char nameOfFile[200];
	unsigned char* eachFile;
	struct eachFile scriptFile;
	FILE* outputFile;

	for(i = 0; i < scriptHeader.numFiles; i++)
	{
		scriptFile = GetDirTree(&dirTree);

		//Fetch the string, prepending it with SCRIPT_PVF
		memcpy(nameOfFile, SCRIPT_PVF, strlen(SCRIPT_PVF));
		memcpy(nameOfFile + strlen(SCRIPT_PVF), scriptFile.fileName, scriptFile.lengthString);
		nameOfFile[strlen(SCRIPT_PVF) + scriptFile.lengthString] = '\0';

		printf("(%d:%d) :: %s\n", i+1, scriptHeader.numFiles, nameOfFile);

		//Get file from internalFiles
		eachFile = GetFileTree(internalFiles, scriptFile);

		//Commit
		CreateNewDirectoryIterate(nameOfFile);
		outputFile = safe_fopen(nameOfFile, "wb");
		fwrite(eachFile, scriptFile.fileLength, 1, outputFile);
		fclose(outputFile);

		//Tidy up
		free(eachFile);
		free(scriptFile.fileName);
	}
	dirTree -= scriptHeader.lengthDirTree;
}

void OutputDiff(unsigned char* dirTree, unsigned char* internalFiles, unsigned char* scriptCache, struct fileHeader scriptHeader)
{
	int i;
	int j;
	int existFlag;
	int eof;
	unsigned int scriptChecksum;
	int scriptNumFiles;
	unsigned int checksum;
	
	char nameOfFile[200];
	char updatedNameOfFile[200];
	unsigned char* scriptNameOfFile;
	unsigned char* eachFile;
	unsigned char* oldFilePtr;
	unsigned char* baseScriptCache;
	struct eachFile scriptFile;
	FILE* oldFile;

	scriptChecksum = GetInt(&scriptCache);
	scriptNumFiles = GetInt(&scriptCache);

	for(i = 0; i < scriptHeader.numFiles; i++)
	{
		scriptFile = GetDirTree(&dirTree);
		eachFile = GetFileTree(internalFiles, scriptFile);

		//Fetch two strings
		//One being prepended with SCRIPT_PVF
		//One being prepended with UPDATED
		memcpy(nameOfFile, SCRIPT_PVF, strlen(SCRIPT_PVF));
		memcpy(updatedNameOfFile, UPDATED, strlen(UPDATED));
		memcpy(nameOfFile + strlen(SCRIPT_PVF), scriptFile.fileName, scriptFile.lengthString);
		memcpy(updatedNameOfFile + strlen(UPDATED), scriptFile.fileName, scriptFile.lengthString);
		nameOfFile[strlen(SCRIPT_PVF) + scriptFile.lengthString] = '\0';
		updatedNameOfFile[strlen(UPDATED) + scriptFile.lengthString] = '\0';

		//Calculate Checksum
		checksum = CalculateChecksum(eachFile, scriptFile.computedFileLength, 0);
		
		//Begin diff searching
		baseScriptCache = scriptCache;
		existFlag = 0;
		//This looping structure is sloooooowwwww
		//It needs to be faster, but how?
		for(j = 0; j < scriptNumFiles; j++)
		{
			scriptNameOfFile = GetString(&scriptCache);
			scriptChecksum = GetInt(&scriptCache);
			if(strcmp(scriptNameOfFile, nameOfFile) == 0)
			{
				existFlag = 1;
				if(scriptChecksum != checksum)
				{
					printf("%s\n", updatedNameOfFile);
					
					//TODO - Redo this section so it doesn't use
					//oldFile as the file pointer over and over
					//to improve readability

					//Read old file
					CreateNewDirectoryIterate(nameOfFile);
					oldFile = safe_fopen(nameOfFile, "rb");
					eof = GetEOF(oldFile);
					oldFilePtr = safe_malloc(eof);
					fread(oldFilePtr, eof, 1, oldFile);
					fclose(oldFile);

					//Replace the old file
					oldFile = safe_fopen(nameOfFile, "wb");
					fwrite(eachFile, scriptFile.fileLength, 1, oldFile);
					fclose(oldFile);

					//Commit the new file
					CreateNewDirectoryIterate(updatedNameOfFile);
					oldFile = safe_fopen(updatedNameOfFile, "wb");
					fwrite(eachFile, scriptFile.fileLength, 1, oldFile);
					fclose(oldFile);

					//Switch the file name to "ORIGINAL" and write the old file
					MakeOriginal(updatedNameOfFile);
					oldFile = safe_fopen(updatedNameOfFile, "wb");
					fwrite(oldFilePtr, eof, 1, oldFile);
					fclose(oldFile);

					free(oldFilePtr);
				}
				break;
			}
			free(scriptNameOfFile);
		}
		//Reset the pointer
		scriptCache = baseScriptCache;

		//This should probably be included somehow in the looping structure above
		//What if the file wasn't found?
		if(existFlag == 0)
		{
			//Okay, so this is a brand new file
			CreateNewDirectoryIterate(nameOfFile);
			oldFile = safe_fopen(nameOfFile, "rb");
			fwrite(eachFile, scriptFile.fileLength, 1, oldFile);
			fclose(oldFile);

			CreateNewDirectoryIterate(updatedNameOfFile);
			oldFile = safe_fopen(updatedNameOfFile, "rb");
			fwrite(eachFile, scriptFile.fileLength, 1, oldFile);
			fclose(oldFile);
		}
		free(eachFile);
	}
	dirTree -= scriptHeader.lengthDirTree;
}

void MakeOriginal(char* nameOfFile)
{
	int i;
	char outputNameOfFile[200];

	//Purpose of this function...
	//"Hello.txt" --> "Helloorig.txt"
	//"foo.bar.txt" --> "foo.barorig.txt"

	for(i = strlen(nameOfFile); nameOfFile[i] != '/'; i--)
	{
		if(nameOfFile[i] == '.')
		{
			memcpy(outputNameOfFile, nameOfFile, i);
			memcpy(outputNameOfFile + i, ORIGINAL, strlen(ORIGINAL));
			memcpy(outputNameOfFile + i + strlen(ORIGINAL), nameOfFile + i, strlen(nameOfFile) - i);
			outputNameOfFile[strlen(nameOfFile) + strlen(ORIGINAL)] = '\0';
			memcpy(nameOfFile, outputNameOfFile, strlen(outputNameOfFile) + 1);
			break;
		}
	}
}

/* END PROCEDURES */