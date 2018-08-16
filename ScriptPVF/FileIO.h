/* FILE READING */
long GetEOF(FILE* PatchFile);
long GetEOFFile(char* fileName);
int rS32(FILE* fileObj);
unsigned char* ReadTheFile(char* fileName);
/* END FILE READING */

/* PLATFORM DEPENDENT */
void CreateNewDirectory(char* fileName); //Possible Windows/UNIX clashing here
void CreateNewDirectoryIterate(char* dirName);
void WriteCarriageReturn(FILE* fileName); //Possible Windows/UNIX clashing here
/* END PLATFORM DEPENDENT */

/* SAFE WRAPPERS */
unsigned char* safe_malloc(int size);
unsigned char* safe_realloc(unsigned char* buffer, int size);
FILE* safe_fopen(char* fileName, char* openType);
/* END SAFE WRAPPERS */