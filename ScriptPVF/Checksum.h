void Generatesbox(unsigned int* sbox);
unsigned int CalculateChecksum(unsigned char* block, int lengthOfBlock, unsigned int curChecksum);
unsigned int CalculateChecksumFile(char* fileName);
void PVFChecksum(unsigned char* block, int lengthOfBlock, unsigned int numFiles, unsigned int checksum);