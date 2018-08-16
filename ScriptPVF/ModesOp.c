#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Checksum.h"
#include "Tokenizer.h"
#include "FileIO.h"
#include "ModesOp.h"

#define BUFFER 25

void Create()
{
	FILE* Script;
	FILE* Cache;
	unsigned char* dirTree;
	unsigned char* internalFiles;
	struct fileHeader scriptHeader;

	//Read and decrypt Script.pvf
	Script = safe_fopen("Script.pvf", "rb");
	scriptHeader = GetHeader(Script);
	dirTree = DecryptDirTree(Script, scriptHeader);
	internalFiles = DecryptFileTree(Script, dirTree, scriptHeader);
	fclose(Script);

	Cache = safe_fopen("ScriptCache.cache", "wb");
	CreateCache(Cache, dirTree, internalFiles, scriptHeader);
	fclose(Cache);

	free(dirTree);
	free(internalFiles);
}

void Unpack()
{
	FILE* Script;
	unsigned char* dirTree;
	unsigned char* internalFiles;
	struct fileHeader scriptHeader;
	FILE* OutputFile;
	
	//Decrypt and output
	Script = safe_fopen("Script.pvf", "rb");
	scriptHeader = GetHeader(Script);
	dirTree = DecryptDirTree(Script, scriptHeader);
	OutputFile = safe_fopen("Script.pvf.bin", "wb");
	fwrite(dirTree, scriptHeader.lengthDirTree, 1, OutputFile);
	fclose(OutputFile);
	exit(EXIT_SUCCESS);
	internalFiles = DecryptFileTree(Script, dirTree, scriptHeader);

	Output(dirTree, internalFiles, scriptHeader);
	fclose(Script);

	free(dirTree);
	free(internalFiles);

	//With a fresh unpack, create a cache file too
	Create();
}

void Diff()
{
	FILE* Script;
	FILE* Cache;
	unsigned char* dirTree;
	unsigned char* internalFiles;
	unsigned char* scriptCache;
	unsigned int checksum;
	unsigned int checksumCheck;
	struct fileHeader scriptHeader;

	checksum = CalculateChecksumFile("Script.pvf");

	Cache = safe_fopen("ScriptCache.cache", "rb");
	fread(&checksumCheck, 4, 1, Cache);
	fclose(Cache);

	if(checksum != checksumCheck)
	{

		Script = safe_fopen("Script.pvf", "rb");
		scriptHeader = GetHeader(Script);
		dirTree = DecryptDirTree(Script, scriptHeader);
		internalFiles = DecryptFileTree(Script, dirTree, scriptHeader);
		fclose(Script);

		scriptCache = ReadTheFile("ScriptCache.cache");
		OutputDiff(dirTree, internalFiles, scriptCache, scriptHeader);

		free(dirTree);
		free(scriptCache);
		free(internalFiles);

		//Also since the files are now updated
		//it makes sense to rebuild the cache
		Create();
	}
}

void Decrypt(void)
{
	FILE* Script;
	FILE* ScriptOutput;
	struct fileHeader scriptHeader;
	unsigned char* dirTree;
	unsigned char* fileTree;

	Script = safe_fopen("Script.pvf", "rb");
	scriptHeader = GetHeader(Script);
	dirTree = DecryptDirTree(Script, scriptHeader);
	fileTree = DecryptFileTree(Script, dirTree, scriptHeader);
	fclose(Script);

	ScriptOutput = safe_fopen("ScriptOutput.pvf", "wb");

	//Header
	fwrite(&scriptHeader.lengthChecksum, 4, 1, ScriptOutput);
	fwrite(scriptHeader.checksum, scriptHeader.lengthChecksum, 1, ScriptOutput);
	fwrite(&scriptHeader.version, 4, 1, ScriptOutput);
	fwrite(&scriptHeader.lengthDirTree, 4, 1, ScriptOutput);
	fwrite(&scriptHeader.dirTreeChecksum, 4, 1, ScriptOutput);
	fwrite(&scriptHeader.numFiles, 4, 1, ScriptOutput);

	//Directory & file tree
	fwrite(dirTree, scriptHeader.lengthDirTree, 1, ScriptOutput);
	fwrite(fileTree, scriptHeader.lengthFileTree, 1, ScriptOutput);

	//Tidy up
	fclose(ScriptOutput);
	free(dirTree);
	free(fileTree);
}