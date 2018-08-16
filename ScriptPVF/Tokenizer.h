/* TOKENS */
struct fileHeader GetHeader(FILE*);
struct eachFile GetDirTree(unsigned char** dirTree);
unsigned int GetInt(unsigned char** fileBlock);
unsigned int GetIntFile(FILE* Script);
unsigned char* GetString(unsigned char** fileBlock);
unsigned char* GetStringFile(FILE* Script);
unsigned char* GetFileTree(unsigned char* internalFiles, struct eachFile scriptFile);

/* DECRYPTION */
unsigned char* DecryptDirTree(FILE* Script, struct fileHeader scriptHeader);
void DecryptBlock(unsigned char* fileBlock, int lengthOfBlock, unsigned int beginningKey);
unsigned char* DecryptFileTree(FILE* Script, unsigned char* dirTree, struct fileHeader scriptHeader);

/* PROCEDURES */
void CreateCache(FILE* Cache, unsigned char* dirTree, unsigned char* internalFiles, struct fileHeader scriptHeader);
void Output(unsigned char* dirTree, unsigned char* internalFiles, struct fileHeader scriptHeader);
void OutputDiff(unsigned char* dirTree, unsigned char* internalFiles, unsigned char* scriptCache, struct fileHeader scriptHeader);
void MakeOriginal(char* nameOfFile);

struct fileHeader
{
	int lengthChecksum;
	unsigned char* checksum;
	int version;
	int lengthDirTree;
	int lengthFileTree;
	unsigned int dirTreeChecksum;
	int numFiles;
	int dirTreeStart;
	int fileTreeStart;
	int eof;
};

struct eachFile
{
	int fileNumber;
	int lengthString;
	char* fileName;
	int fileLength;
	unsigned int checksum;
	int relOffset;
	int computedFileLength;
};


