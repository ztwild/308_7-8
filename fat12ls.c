/* fat12ls.c 
* 
*  Displays the files in the root sector of an MSDOS floppy disk
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define SIZE 32      /* size of the read buffer */
#define ROOTSIZE 256 /* max size of the root directory */
//define PRINT_HEX   // un-comment this to print the values in hex for debugging

struct BootSector{
  unsigned char  sName[9];            // The name of the volume
  unsigned short iBytesSector;        // Bytes per Sector

  unsigned char  iSectorsCluster;     // Sectors per Cluster
  unsigned short iReservedSectors;    // Reserved sectors
  unsigned char  iNumberFATs;         // Number of FATs

  unsigned short iRootEntries;        // Number of Root Directory entries
  unsigned short iLogicalSectors;     // Number of logical sectors
  unsigned char  xMediumDescriptor;   // Medium descriptor

  unsigned short iSectorsFAT;         // Sectors per FAT
  unsigned short iSectorsTrack;       // Sectors per Track
  unsigned short iHeads;              // Number of heads

  unsigned short iHiddenSectors;      // Number of hidden sectors
};

void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[]);
//  Pre: Calculated directory offset and number of directory entries
// Post: Prints the filename, time, date, attributes and size of each entry

unsigned short endianSwap(unsigned char one, unsigned char two);
//  Pre: Two initialized characters
// Post: The characters are swapped (two, one) and returned as a short

void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[]);
//  Pre: An initialized BootSector struct and a pointer to an array
//       of characters read from a BootSector
// Post: The BootSector struct is filled out from the buffer data

char * toDOSName(char string[], unsigned char buffer[], int offset);
//  Pre: String is initialized with at least 12 characters, buffer contains
//       the directory array, offset points to the location of the filename
// Post: fills and returns a string containing the filename in 8.3 format

char * parseAttributes(char string[], unsigned char key);
//  Pre: String is initialized with at least five characters, key contains
//       the byte containing the attribue from the directory buffer
// Post: fills the string with the attributes

char * parseTime(char string[], unsigned short usTime);
//  Pre: string is initialzied for at least 9 characters, usTime contains
//       the 16 bits used to store time
// Post: string contains the formatted time

char * parseDate(char string[], unsigned short usDate);
//  Pre: string is initialized for at least 13 characters, usDate contains
//       the 16 bits used to store the date
// Post: string contains the formatted date

int roundup512(int number);
// Pre: initialized integer
// Post: number rounded up to next increment of 512


// reads the boot sector and root directory
  int main(int argc, char * argv[]){
  int pBootSector = 0;
  unsigned char buffer[SIZE];
  unsigned char rootBuffer[ROOTSIZE * 32];
  struct BootSector sector;
  int iRDOffset = 0;

  // Check for argument
  if (argc < 2) {
    printf("Specify boot sector\n");
    exit(1);
  }

  // Open the file and read the boot sector
  pBootSector = open(argv[1], O_RDONLY);
  read(pBootSector, buffer, SIZE);

  // Decode the boot Sector
  decodeBootSector(&sector, buffer);

  // Calculate the location of the root directory
  iRDOffset = (1 + (sector.iSectorsFAT * sector.iNumberFATs) ) * sector.iBytesSector;

  // Read the root directory into buffer
  lseek(pBootSector, iRDOffset, SEEK_SET);
  read(pBootSector, rootBuffer, ROOTSIZE);
  close(pBootSector);

  // Parse the root directory
  parseDirectory(iRDOffset, sector.iRootEntries, rootBuffer);
    
} // end main


// Converts two characters to an unsigned short with two, one
unsigned short endianSwap(unsigned char one, unsigned char two){
    unsigned short s = two << 8 | one;
    return s;
}


// Fills out the BootSector Struct from the buffer
void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[]){
  int i = 3;  // Skip the first 3 bytes

  // Pull the name and put it in the struct (remember to null-terminate)
  int j;
  for(j = 0; j < 8; j++){
    pBootS->sName[j] = buffer[j + i];
  }
  
  // Read bytes/sector and convert to big endian
  pBootS->iBytesSector  = endianSwap(buffer[11], buffer[12]);

  // Read sectors/cluster, Reserved sectors and Number of Fats
  pBootS->iSectorsCluster = buffer[13];
  pBootS->iReservedSectors = endianSwap(buffer[14], buffer[15]);    // Reserved sectors
  pBootS->iNumberFATs = buffer[16];                                 // Number of FATs
  
  // Read root entries, logicical sectors and medium descriptor
  pBootS->iRootEntries = endianSwap(buffer[17], buffer[18]);        // Number of Root Directory entries
  pBootS->iLogicalSectors = endianSwap(buffer[19], buffer[20]);     // Number of logical sectors
  pBootS->xMediumDescriptor = buffer[21];                           // Medium descriptor
  
  // Read and covert sectors/fat, sectors/track, and number of heads
  pBootS->iSectorsFAT = endianSwap(buffer[22], buffer[23]);         // Sectors per FAT 
  pBootS->iSectorsTrack = endianSwap(buffer[24], buffer[25]);       // Sectors per Track 
  pBootS->iHeads = endianSwap(buffer[26], buffer[27]);              // Number of heads 
  
  // Read hidden sectors
  pBootS->iHiddenSectors = endianSwap(buffer[28], buffer[29]);      // Number of hidden sectors 
  
  return;
}


// iterates through the directory to display filename, time, date,
// attributes and size of each directory entry to the console
void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[]){
    int i = 0;
    char string[13];
    printf("There are %d entries\n", iEntries);
    // Display table header with labels
    printf("Filename\tAttrib\tTime\t\tDate\t\tSize\n");

    // loop through directory entries to print information for each
    //for(i = 0; i < (iEntries); i = i + /* entry width */)   {
    for(i = 0; i < (iEntries); i = i + 32)   {
      if (buffer[i] != 0x00 && buffer[i] != 0xE5) {
        // Display filename
        printf("%s\t", toDOSName(string, buffer, iDirOff)  ); /*name offset*/
        // Display Attributes
        printf("%s\t", parseAttributes(string, buffer[iDirOff + 11])  ); /* attr offset */
        // Display Time
        printf("%s\t", parseTime(string, endianSwap(buffer[iDirOff + 22], buffer[iDirOff + 23]) ));  /*time offsets */
        // Display Date
        printf("%s\t", parseDate(string, endianSwap(buffer[iDirOff + 24], buffer[iDirOff + 25]) )); /*date offsets */
        // Display Size
        unsigned short first =endianSwap( buffer[iDirOff + 28], buffer[iDirOff + 29] );
        unsigned short second = endianSwap( buffer[iDirOff + 30], buffer[iDirOff + 31] );
        int size = ( second >> 16 ) | first;
        printf("%d\n", size); /* size offsets */
      }
    }

    // Display key
    printf("(R)ead Only (H)idden (S)ystem (A)rchive\n");
} // end parseDirectory()


// Parses the attributes bits of a file
char * parseAttributes(char string[], unsigned char key){
    int i, j = 0;
    char keys[] = {'R', 'H', 'S', '\0', 'A'};

    //printf("key: 0x%02x, ", key);
    for(i = 0; i < 5; i++){
        if(key % 2 == 1 && i != 3){
            string[j++] = keys[i];
        }
        key = key >> 1;
    }
   // printf("string: %s\n", string);
    // while(j < 13){
    //     string[j++] = '\0';
    // }
    return string;
} // end parseAttributes()


// Decodes the bits assigned to the time of each file
char * parseTime(char string[], unsigned short usTime){
    unsigned char hour = 0x00, min = 0x00, sec = 0x00;
    
    // DEBUG: printf("time: %x", usTime);
    
    sec = (usTime & 31) * 2;
    min = (usTime >> 5) & 63;
    hour = (usTime >> 11) & 31;

    // This is stub code!

    sprintf(string, "%02i:%02i:%02i", hour, min, sec);

    return string;

} // end parseTime()


// Decodes the bits assigned to the date of each file
char * parseDate(char string[], unsigned short usDate){
    unsigned char month = 0x00, day = 0x00;
    unsigned short year = 0x0000;

    //printf("date: %x", usDate);
    
    day = (usDate & 31);
    month = (usDate >> 5) & 15;
    year = (usDate >> 9) & 127;

    sprintf(string, "%d/%d/%d", year, month, day);

    return string;

} // end parseDate()


int roundup512(int number){
  if(number % 512 != 0){
    //printf("Original number: %d\n", number);
    number = (number / 512) + 1 * 512;
    //printf("roundup 512: %d\n", number);
  }
  return number;
}

// Formats a filename string as DOS (adds the dot to 8-dot-3)
char * toDOSName(char string[], unsigned char buffer[], int offset){
    //printf("starting at %d\n", offset);
    int i = 0, j;
    //printf("Value of first: 0x%02x\n", buffer[i + offset]);
    //printf("is equal to 0x20: %d\n", buffer[i + offset] != 0x20);
    
    while(buffer[i + offset] != 0x20 || i == 8){
      string[i] = buffer[i + offset];
      i++;
    }
    string[i++] = '.';
    for(j = 8; i < 11; i++){
      string[i] = buffer[j + offset];
    }
    string[i] = '\0';
    return string;
} // end toDosNameRead-Only Bit
