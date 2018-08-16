#include <stdio.h>
#include <stdlib.h>

#include "Checksum.h"
#include "FileIO.h"
#include "Tokenizer.h"

#define SIZESBOX 256
#define BLOCKSIZE 0x80000

unsigned int CalculateChecksum(unsigned char* block, int lengthOfBlock, unsigned int curChecksum)
{
	unsigned int sbox[SIZESBOX];
	int IndexLookup;
	int blockPos;

	Generatesbox(sbox);

	for(blockPos = 0; blockPos < lengthOfBlock; blockPos++)
	{
		IndexLookup = (curChecksum >> 0x18) ^ block[blockPos];
		curChecksum = (curChecksum << 0x08) ^ sbox[IndexLookup];
	}
	return curChecksum;
}

void PVFChecksum(unsigned char* block, int lengthOfBlock, unsigned int curChecksum, unsigned int finalChecksum)
{
	int i;
	unsigned int sbox[SIZESBOX];
	unsigned int EAX = 0;
	unsigned int EDX = 0;
	unsigned int ESI = 0;
	unsigned int ECX = 0;

	Generatesbox(sbox);

	//The following is directly copied from the DFO.exe file
	//In the EXE file, the "DecryptBlock" procedure and this
	//checksum procedure are intermingled. I've only decided
	//to separate them for simplicity.

	//If anyone can figure out a much more simplified version
	//of this, please let me know! ~Fiel

	EAX = finalChecksum;
	ESI = ~curChecksum;
	for(i = 0; i < lengthOfBlock; i+= 4)
	{
		ECX = GetInt(&block);

		//xor     edx, edx
		EDX ^= EDX;

		//mov     dl, cl
		EDX = ECX & 0xFF;

		//mov     eax, esi
		EAX = ESI;

		//shr     eax, 8
		EAX >>= 8;

		//xor     edx, esi
		EDX ^= ESI;

		//and     edx, 0FFh
		EDX &= 0xFF;

		//mov     esi, dword ptr byte_1004798[edx*4]
		ESI = sbox[EDX];

		//xor     edx, edx
		EDX ^= EDX;

		//mov     dl, [ecx - 3]
		EDX = (ECX >> 0x08) & 0xFF;

		//xor     eax, esi
		EAX ^= ESI;

		//xor     edx, eax
		EDX ^= EAX;

		//and     edx, 0FFh
		EDX &= 0xFF;

		//mov     esi, dword ptr byte_1004798[edx*4]
		ESI = sbox[EDX];

		//xor     edx, edx
		EDX ^= EDX;

		//mov     dl, [ecx - 2]
		EDX = (ECX >> 16) & 0xFF;

		//shr     eax, 8
		EAX >>= 8;

		//xor     eax, esi
		EAX ^= ESI;

		//xor     edx, eax
		EDX ^= EAX;

		//and     edx, 0FFh
		EDX &= 0xFF;

		//mov     esi, dword ptr byte_1004798[edx*4]
		ESI = sbox[EDX];

		//xor     edx, edx
		EDX ^= EDX;

		//mov     dl, [ecx - 1]
		EDX = (ECX >> 24) & 0xFF;

		//shr     eax, 8
		EAX >>= 8;

		//xor     eax, esi
		EAX ^= ESI;

		//xor     edx, eax
		EDX ^= EAX;

		//and     edx, 0FFh
		EDX &= 0xFF;

		//mov     esi, dword ptr byte_1004798[edx*4]
		ESI = sbox[EDX];

		//shr     eax, 8
		EAX >>= 8;

		//xor     eax, esi
		EAX ^= ESI;

		//xor     edx, eax
		EDX ^= EAX;

		//mov     esi, eax
		ESI = EAX;
	}

	curChecksum = ~ESI;

	if(finalChecksum != curChecksum)
	{
		printf("Checksum error!\n");
		exit(EXIT_FAILURE);
	}
}

//Wrapper/convenience function for "CalculateChecksum".
//It will calculate the checksum of the file given by "fileName"
unsigned int CalculateChecksumFile(char* fileName)
{
	unsigned char* block;
	unsigned int checksum = 0;
	int sizeOfBlock;
	FILE* wzFile;
	long eof;
	int i;

	sizeOfBlock = BLOCKSIZE;
	block = safe_malloc(BLOCKSIZE);
	wzFile = safe_fopen(fileName, "rb");
	eof = GetEOF(wzFile);

	for(i = 0; i < eof; i += BLOCKSIZE)
	{
		//This if statement ensures that the program does not
		//go past feof or create out of bounds errors
		if(i + BLOCKSIZE > eof)
		{
			sizeOfBlock = eof - i;
		}
		fseek(wzFile, i, SEEK_SET);
		fread(block, sizeOfBlock, 1, wzFile);
		checksum = CalculateChecksum(block, sizeOfBlock, checksum);
	}
	fclose(wzFile);
	free(block);
	return checksum;
}

void Generatesbox(unsigned int* sbox)
{
	unsigned int const Polynomial = 0xEDB88320L;
	unsigned int remain;
	unsigned int dividend;
	unsigned int bit;

	for(dividend = 0; dividend < SIZESBOX; dividend++)
	{
		remain = dividend;
		for(bit = 0; bit < 8; bit++)
		{
			if(remain & 1)
			{
				remain = (remain >> 1) ^ Polynomial;
			}
			else
			{
				remain = (remain >> 1);
			}
		}
		sbox[dividend] = remain;
	}
}