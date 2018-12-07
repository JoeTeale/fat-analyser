#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* standard offset positions/associations, makes for readable reference, could be also
be easily altered for variants if needed */
#define		SIZE_OF_FAT 0x16
#define 	NUM_OF_FATS 0x10
#define		SIZE_OF_DISK 0x13
#define 	OVERFLOW_SIZE_OF_DISK 0x20
#define		RESERVED_SPACE 0x0e
#define 	NUM_OF_ROOT_ENTRIES 0x11
#define		FILE_SIZE_RELATIVE 0x1c
#define 	FILE_EXTENSION_RELATIVE 0x08
#define		BLOCKS_PER_A_UNIT 0x0d
/* type define for the boot block of img file, used to declare
pointer for use in functions expecting pointer (such as fread), makes for a
memorable/logical struct of vars, removes some repeitions of realloc,
easier to pass to functions than a conventonal struct */
typedef struct{
  uint8_t numofFats;
  uint16_t sizeofFats;
  uint32_t sizeofDisk;
  uint16_t reservedSpace;
  uint16_t numofRootEntries;
  uint8_t blocksPerAllocationUnit;
} bootBlock;
/* similar to boot block but for root file of img */
typedef struct{
  uint16_t startofRoot;
  uint32_t sizeofRoot;
  char fileName[64];
  char givenFilesExtension[24];
  char unit[64];
  uint16_t unitDate;
  uint16_t unitYear;
  uint16_t unitMonth;
  uint16_t unitDay;
  char garg[64];
  uint16_t gargTime;
  uint16_t gargHours;
  uint16_t gargMinutes;
  uint16_t gargSeconds;
  char birth[64];
  uint16_t birthStartCluster;
} root;
typedef struct{
	uint16_t nextBlockOne;
	uint16_t nextBlockTwo;
}  fatTable;
/* trim white space of string */ 
void trim(char* source)
{
  char* i = source;
  char* j = source;
  while(*j != 0)
  {
    *i = *j++;
    if(*i != ' ')
      i++;
  }
  *i = 0;
}
/* used to populate the root struct vars,each populate function seperated primarily
for readability reasons and ease of reference */
int populateRoot(FILE *targetFile, root *rootDir, bootBlock *bb)
{
	
   int  a;
   /* determine start of root byte of root, useful for finding root entries */
   rootDir->startofRoot = ((bb->sizeofFats * bb->numofFats) + 1) * 512;
   /* determine the size of root by multiplying number of entires in root
      by the size of one entry (according to Assignment provided website)
      of 32 bytes */
   rootDir->sizeofRoot = (bb->numofRootEntries * 32);
   if (fseek(targetFile,rootDir->startofRoot,SEEK_SET) == -1) {
     return -1;
   }
   /* fseek to ninth entry in root from current cur position */
   if (fseek(targetFile,8*32,SEEK_CUR) == -1) {
     return -1;
   }
   /* captures the file name in a char of 64 bytes */
   if (fread(rootDir->fileName, sizeof(char), 8, targetFile) < 1) {
    return -2;
   }
   /* smaller char array of 24 bytses for file extension */
   if (fread(rootDir->givenFilesExtension, sizeof(char), 3, targetFile) < 1){
    return -2;
   }
   if (fseek(targetFile,rootDir->startofRoot,SEEK_SET) == -1) {
    return -1;
   }
   /* loop through total number of entries based on a 32 byte size giving root size in bytes */
   for(a = 0; a < rootDir->sizeofRoot; a=a+32){
     if (fseek(targetFile, rootDir->startofRoot + a, SEEK_SET) == -1) {
       return -1;
     }
	 /* captures string name for comparison */
     if (fread(rootDir->unit, sizeof(char), 8, targetFile) < 1) {
       return -2;
     }
	 /* string we are looking for is larger enough to ignore the need to trim for str comparison */ 
     if (strcmp(rootDir->unit,"UNITS666") == 0){
       if (fseek(targetFile,16,SEEK_CUR) == -1) {
	 return -1;
       }
       if (fread(&rootDir->unitDate, sizeof(uint16_t), 1, targetFile) < 1) {
	 return -2;
       }
	   /* shift the bits by 9 bits to the right, dropping all bits unrelated to the Year element of the 16 bit value */
       rootDir->unitYear = rootDir->unitDate >> 9;
	   /* bitmask using and operator, essentially a 1-1 mapping of bits, where 0 will return 0 as 1-0 or 0-0 does not return true, and 1 will retain the previous value, then shift right to drop the day portion of the value */
       rootDir->unitMonth = (rootDir->unitDate & 0b0000000111100000) >> 5;
	   /* no shift this time as only concerend with last 5 bits */ 
       rootDir->unitDay = rootDir->unitDate & 0b000000000001111;
       break;
     }
   }
   /* same loop logic as before */ 
   for(a = 0; a < rootDir->sizeofRoot; a=a+32){
        if (fseek(targetFile,rootDir->startofRoot+a,SEEK_SET) == -1) {
         return -1;
        }
		/* 64 byte file name */ 
        if (fread(rootDir->garg, sizeof(char), 8, targetFile) < 1) {
         return -2;
        }
		/* again no need for trim due to size of target file name */ 
	if (strcmp(rootDir->garg,"GARGRAVR") == 0){
                if (fseek(targetFile,14,SEEK_CUR) == -1) {
                 return -1;
                }
                if (fread(&rootDir->gargTime, sizeof(uint16_t), 1, targetFile) < 1) {
                return -2;
                }
				/* same logic as date, but concered with differents bit */ 
                rootDir->gargHours = rootDir->gargTime >> 11;
                rootDir->gargMinutes = (rootDir->gargTime & 0b0000011111100000) >> 5;
                rootDir->gargSeconds = rootDir->gargTime & 0b0000000000011111;
                break;
        }
   }
   for(a = 0; a < rootDir->sizeofRoot; a=a+32){
        if (fseek(targetFile,rootDir->startofRoot+a,SEEK_SET) == -1) {
         return -1;
        }
        if (fread(rootDir->birth, sizeof(char), 8, targetFile) < 1) {
         return -2;
        }
		/* File name too short on this occasion to fill allocated spcae in img, so trim removes array elements that equal ' '*/
	trim(rootDir->birth);
        if (strcmp(rootDir->birth,"BIRTH") == 0){
                if (fseek(targetFile,18,SEEK_CUR) == -1) {
                 return -1;
                }
                if (fread(&rootDir->birthStartCluster, sizeof(uint16_t), 1, targetFile) < 1){
                return -2;
                }
                break;
        }
   }
   return 0;
}
/* same purpose as populateRoot, but for the bootBlock */
int populateBootBlock(FILE *targetFile, bootBlock *bb)
{
  /* the boot block has known offsets, thus these have been defined at the start of the programme */
  /* same logic as other seeks, only this time we have an easier offset to find, rather than calulating the number of required bytes */ 
  /* NUM_OF_FATS */
  if (fseek(targetFile, NUM_OF_FATS,SEEK_SET) == -1) {
    return -1;
  }
  if (fread(&bb->numofFats, sizeof(uint8_t), 1, targetFile) < 1) {
    return -2;
  }
  /* find the size of a fat */ 
  if (fseek(targetFile, SIZE_OF_FAT,SEEK_SET) == -1) {
    return -1;
  }
  if (fread(&bb->sizeofFats, sizeof(uint16_t), 1, targetFile) < 1) {
    return -2;
  }
  /* find the size of disk */
  if (fseek(targetFile, SIZE_OF_DISK,SEEK_SET) == -1) {
    return -1;
  }
  if (fread(&bb->sizeofDisk, sizeof(uint16_t), 1, targetFile) < 1) {
    return -2;
  }
  /* if the size of disk to large to fit in 2 bytes, its recorded as 0 and instead found at 0x20 (OVERFLOW_SIZE_OF_DISK) */ 
  if (&bb->sizeofDisk == 0){

	if (fseek(targetFile, OVERFLOW_SIZE_OF_DISK,SEEK_SET) == -1) {
   		 return -1;
 	}/* secondary location is 4 bytes rather than 2 */ 
	if (fread(&bb->sizeofDisk, sizeof(uint32_t), 1, targetFile) < 1) {
    		return -2;
	}
  }
  /* resereved spcae, typically returns 1 as only usualy bootblock */
  if (fseek(targetFile, RESERVED_SPACE,SEEK_SET) == -1) {
    return -1;
  }
  if (fread(&bb->reservedSpace, sizeof(uint16_t), 1, targetFile) < 1) {
    return -2;
  }
  /* NUM_OF_ROOT_ENTRIES used to determine the size of root */ 
  if (fseek(targetFile, NUM_OF_ROOT_ENTRIES,SEEK_SET) == -1) {
    return -1;
  }
  if (fread(&bb->numofRootEntries, sizeof(uint16_t), 1, targetFile) < 1) {
    return -2;
  }
  /* returns only one for this file, however is an important consideration for fats that allocate using larger allocation units sizes */ 
  if (fseek(targetFile, BLOCKS_PER_A_UNIT,SEEK_SET) == -1) {
    return -1;
  }
  if (fread(&bb->blocksPerAllocationUnit, sizeof(uint8_t), 1, targetFile) < 1) {
    return -2;
  }
  return 0;
}

 /* same agian this data is primary for the BIRTH.tok file */
int populateFat(FILE *targetFile, fatTable *fatTb, root *rootDir)
{
	/* each entrie is two bytes in size so * 2 the cluster */
	uint16_t fatOffset = rootDir->birthStartCluster * 2;
	/* gives us the block from fat +1 as bootBlock is actuall in the first block */
	uint16_t fatBlock = fatOffset /0x200;
	uint16_t diskBlock = fatBlock + 1; 
	/* gives us the position in the relative block */ 
	uint16_t offsetInBlock = fatOffset % 0x200;
	/* used to convert cluster number to a block number */ 
	uint16_t rootOffset = (rootDir->sizeofRoot /512) + (rootDir->startofRoot / 512) - 2;
	
	if (fseek(targetFile,diskBlock * 0x200,SEEK_SET) == -1) {
	return -1;
	}
	if (fseek(targetFile,offsetInBlock,SEEK_CUR) == -1) {
	return -1;
	}
	if (fread(&fatTb->nextBlockOne, sizeof(uint16_t), 1, targetFile) < 1) {
    return -2;
    }
	/* poor practice, but repeated code as didn't have time to pass it too a new function and recall, luckily only needed it twice */
	fatOffset = fatTb->nextBlockOne * 2;
	fatBlock = fatOffset /0x200;
	diskBlock = fatBlock + 1; 
	offsetInBlock = fatOffset % 0x200;
	rootOffset = (rootDir->sizeofRoot /512) + (rootDir->startofRoot / 512) - 2;
	if (fseek(targetFile,diskBlock * 0x200,SEEK_SET) == -1) {
	return -1;
	}
	if (fseek(targetFile,offsetInBlock,SEEK_CUR) == -1) {
	return -1;
	}
	if (fread(&fatTb->nextBlockTwo, sizeof(uint16_t), 1, targetFile) < 1) {
    return -2;
    }
	/* the cluser + rootOffst gives us the block where we can find the relevant data */ 
    fatTb->nextBlockOne = (fatTb->nextBlockOne + rootOffset);
	fatTb->nextBlockTwo = (fatTb->nextBlockTwo + rootOffset);
	return 0;
}
/* prints and formats data found in the relevnt pointers of particular struct types */ 
void report(bootBlock *bb, root *rootDir, fatTable *fatTb) {
  printf("* Boot block data\n");
  printf("\tSize of file system: %d (%#x) blocks\n",(bb->sizeofDisk - bb->reservedSpace),(bb->sizeofDisk - bb->reservedSpace)); 
  printf("\tNumber of File Allocation Tables: %d (%#x)\n", bb->numofFats, bb->numofFats );
  printf("\tSize of fat tables: %d (%#x) blocks\n", bb->sizeofFats, bb->sizeofFats);
  printf("* Root Data\n");
  printf("\tSize of Root:%d (%#x) blocks\n", (rootDir->sizeofRoot / 512), (rootDir->sizeofRoot / 512));
  trim(rootDir->fileName);
  printf("\tThe ninth entry is called: %s.%s\n", &rootDir->fileName,&rootDir->givenFilesExtension);
  /* simple way of trimming an int without conversion to a string */ 
  rootDir->unitYear+=1980;
  if (rootDir->unitYear >= 2000){
	rootDir->unitYear-=2000;
  }
  else{
	rootDir->unitYear-=1900;
  }
  printf("\tThe date of the file UNITS666 is: %02d/%02d/%02d\n",rootDir->unitDay,rootDir->unitMonth,rootDir->unitYear);
  printf("\tGARGRAVR time created: %02d:%02d:%02d\n",rootDir->gargHours, rootDir->gargMinutes, rootDir->gargSeconds);
  printf("* BIRTH.TOK data \n ");
  uint16_t startBlockOfBirth = (rootDir->birthStartCluster + (rootDir->startofRoot /512) + (rootDir->sizeofRoot / 512) - 2);
  printf("\tBIRTH.TOK data starts at block %d (%#x)\n",  startBlockOfBirth , startBlockOfBirth );
  printf("\tThe second block is %d (%#x)\n", fatTb->nextBlockOne, fatTb->nextBlockOne);
  printf("\tThe third block is %d (%#x) \n", fatTb->nextBlockTwo, fatTb->nextBlockTwo);
 }
 /* function calls and error return handling */ 
int main () {
  FILE *fp;
  bootBlock *bb;
  root *rootDir;
  fatTable *fatTb;
  /* open the file pointer */  
  fp = fopen("testdisk.img","r");
  if (!fp) {
    perror("Error opening testdisk.img");
    return(EXIT_FAILURE);
  }
  /* allocate memory for bootBlock pointer */ 
  bb = calloc(sizeof(bootBlock), 1);
  if (populateBootBlock(fp, bb) != 0) {
    perror("Error reading boot block from image");
    return(EXIT_FAILURE);
  }
  /* allocate memory for root pointer */ 
  rootDir = calloc(sizeof(root), 1);
  if (populateRoot(fp, rootDir, bb) != 0) {
    perror("Error reading root from image");
    return(EXIT_FAILURE);
  }
  fatTb = calloc(sizeof(fatTable), 1);
  if (populateFat(fp, fatTb, rootDir) != 0) {
    perror("Error reading FAT table from image");
    return(EXIT_FAILURE);
  }
  /* close file to avoid conflict */ 
  if (fclose(fp) != 0) {
    perror("Error closing image file");
    return(EXIT_FAILURE);
  }
  report(bb, rootDir, fatTb);
  /* release allocated memory */ 
  free(bb);
  free(rootDir);
  free(fatTb);
  return(EXIT_SUCCESS);
}
