/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+																		+
+ HideTSR .C															+
+																		+
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include<stdio. h>
#include<string. h>

/* [Data Types]-------------------------------------*/

#define WORD unsigned short
#define BYTE unsigned char

#define SZ_MCB 16		//1 paragraph = 16 bytes = 10H bytes
#define SZ_NAME 8		//1 file name's 8 chars max

/* [Structures]-------------------------------------*/
/*
DOS refers to its memory as an "Arena"
It divides this arena into blocks of memory
Each block starts with an MCB (Memory Control Block, aka Memory Control Record)

[MCB][Memory Block], [MCB][Memory Block], [MCB][Memory Block], ...
*/
struct MCB
{
	BYTE type;			// 'M' normally, 'z' is last entry
	WORD owner;			// Segment address of owner's PSP (exOOOOH == free)
	WORD size;			// Size of MCB ( in 16- byte paragraphs)
	BYTE field[3];	// I suspect this is filler
	BYTE name [SZ_NAME]; // Name of program (environment blocks aren't named)
};

#define MCB_TYPE_NOTEND 'M'
#define MCB_TYPE_END 'Z'

//This structure stor es a far pointer (don't want to r ely on compiler extensions)
struct Address
{
	WORD segment ;
	WORD offset;
};

//This puts the MCB header and its address under a common structure
struct MCBHeader
{
	struct MCB mcb;
	struct Address address ;
};/*[Functions]-------------------------------------------------*/

void printMCB(struct MCB bInfo)
{
	BYTE fileName [SZ_NAME+1] ;
	int i;
	
	//guarantee that this string is safe to print
	fileName[SZ_NAME]= '\0';
	
	printf("Type=%c\t", blnfo.type);
	printf("Owner=%04X\t" , blnfo.owner);
	printf("Size=%04X\t" , blnfo.size);
	printf("Name=");
	
	printf("(");
	if(blnfo.owner==0x0)
	{
		printf("*Free*");
	}
	else if(strlen(fileName)==SZ_NAME)
	{
		//if the null terminator is ours, then it' s probably not a file
		printf( "Environment" );
	}
	else
	{
		for(i=0;i<SZ_NAME;i++){ fileName[i] = blnfo.name[i]; }
		printf( "%s", fileName);
	}
	printf(")");
	printf("\n");
	return;
}/*printMCB---------------------------------------------------------------*/
/*
This takes an array of two bytes and converts them into a WORD
*/
WORD arrayToWord(BYTE *bPair)
{
	WORD *wptr;
	WORD value ;
	
	wptr = (WORD*)bPair;
	value = *wptr;
	return (value) ;
}/*end arrayToWord()--------------------------------------------------*/

/*
Given the address of the MCB header, populate an MCB structure for it
*/
struct MCBHeader populateMCB(struct Address addr)
{
	WORD segment;
	WORD index;
	
	BYTE buffer[SZ_MCB];	//receives the 16 bytes that make up the MCB
	BYTE bytePair[2];		// used to build WORD fields i n the MCB
	BYTE data;				// used within asm-block to get data
	int i,j;
	WORD value;
	
	struct MCBHeader hdr;
	
	//already have the address of the MCB
	
	(hdr.address).segment = addr.segment;
	(hdr.address).offset = addr.offset;

	//do the following to make the asm-block easier to read
	
	segment = addr.segment;
	index = addr.offset;
	
	// iterate through memory to get the bytes into buffer [ )
	for(i=0; i<SZ_MCB; i++)
	{
		_asm
		{
			PUSH ES
			PUSH BX
			PUSH AX
			MOV ES, segment
			MOV BX, index
			MOV AL, ES:[BX]
			MOV data, AL
			POP AX
			POP BX
			POP ES
		}
		buffer[i] = data;
		index++;
	}
	
	//step through the buffer and populate the structure fields
	
	(hdr.mcb).type = buffer[0];
	
	//Nota Bene : the owner's segment address bytes are reversed!
	
	bytePair[0] = buffer[2];
	bytePair[l] = buffer[l];
	value = arrayToWord(bytePair);
	(hdr.mcb).owner = value;
	
	bytePair[0] = buffer[3];
	bytePair[l] = buffer[4];
	value = arrayToWord(bytePair);
	(hdr.mcb).size = value;
	
	for(i=8;i <=l5;i++)
	{
		j = i-8;
		(hdr.mcb).name[j] = buffer[i];
	}
	return(hdr);
}
/*end populateMCB-----------------------------------------------*/

void printArenaAddress(WORD segment, WORD offset)
{
printf( "Arena[CS, IP]=[%04X,%04X]: ",segment,offset);
return;
}/*end printArenaAddress-----------------------------------------*/

/*
Getting your hands on the first MCB is the hard part
	Must use an 'undocumented' DOS system call (function 0x52)
*/
struct MCBHeader getFirstMCB()
{
	// address of "List of File Tables"
	WORD FTsegment;
	WORD FToffset;
	
	//address of first MCB
	WORD headerSegment;
	WORD headerOffset;
	
	struct Address hdrAddr;
	struct MCBHeader mcbHdr;
	/*
	INT 0x21, function 0x52, returns a pointer to a pointer
	Puts address of "List of File Tables" in ES:BX
	Address of first Arena Header is in ES : [BX-4]
	Address is in IP :CS format! (not CS:IP)
	*/
	_asm
	{
		MOV AH,0x52
		INT 0x21
		SUB BX,4
		MOV FTsegment, ES
		MOV FToffset, BX
		MOV AX, ES:[BX]
		MOV headerOffset,AX
		INC BX
		INC BX
		MOV AX, ES:[BX]
		MOV headerSegment, AX
	}
	
	hdrAddr.segment = headerSegment;
	hdrAddr.offset = headerOffset;
	/*
		This should be right near the start of DOS system data
		Can verify these results in two ways :
		1) mem / d (address should be start of system data segment)
		2) debug -d xxxx:xxxx should have ' M' as first char in dump
	*/
	printf( "File Table Address [CS, IP] =%04X, %04X\n" , FTsegment, FToffset) ;
	printf("-----------------------------------------------------------n");
	printArenaAddress (headerSegment, headerOffset) ;
	mcbHdr = populateMCB(hdrAddr);
	return(mcbHdr);
}/*end getFirstMCB--------------------------------------*/
/*
The MCB is the first paragraph of each memory block
To find it, we perform the following calculation :
Address next MCB = address current MCB + size of MCB + size of current block
Offset address is always 0x0000, so we can ignore it
|<-------------------------->|
[MCB] [ Block ] [MCB][ Block ]
*/
struct MCBHeader getNextMCB(struct Address currentAddr, struct MCB currentMCB)
{
	WORD nextSegment ;
	WORD nextOffset;
	
	struct MCBHeader newHeader;
	
	nextSegment = currentAddr.segment;
	nextOffset = 0x0000;
		
	// use current address and size to find next MCB header
	nextSegment = nextSegment + 1; 					// MCB is 1 paragraph
	nextSegment = nextSegment + currentMCB.size;	// block is 'n ' paragraphs
	
	printArenaAddress(nextSegment, nextOffset);
	
	(newHeader.address).segment = nextSegment;
	(newHeader.address).offset = nextOffset;
		
	newHeader = populateMCB(newHeader.address);
	return (newHeader) ;
}/*end getNextMCB-----------------------------------------*/

/*
Update memory so current MCB is skipped over the next time the chain is walked
*/
void hideApp(struct MCBHeader oldHdr, struct MCBHeader currentHdr)
{
	WORD segmentFix;
	WORD sizeFix;
	
	segmentFix = (oldHdr.address).segment;
	sizeFix = (oldHdr.mcb).size + 1 + (currentHdr.mcb).size;
	
	_asm
	{
		PUSH BX
		PUSH ES
		PUSH AX
		
		
		MOV BX, segmentFix
		MOV ES,BX
		MOV BX,0x0
		ADD BX,0x3
		MOV AX, sizeFix
		MOV ES: [BX],AX
		PDP AX
		PDP ES
		PDP BX
	}
	return;
}/*end hideApp()-------------------------------------------*/

/*
Can duplicate MCB chain traversal via debug.exe
Files starting with "$$" are hidden (show via "mem /c" command)
There are telltale signs with "mem Id"
*/
void main ()
{
	struct MCBHeader mcbHeader;
	struct MCBHeader oldHeader;	
	//DOS System Data (i.e, "SD") will always be first in the MCB chain
	mcbHeader = getFirstMCB();
	oldHeader = mcbHeader;
	
	printMCB(mcbHeader.mcb) ;
	while (((mcbHeader.mcb).type != MCB_TYPE_END)&&((mcbHeader.mcb).type == MCB_TYPE_NOTEND))
	{
		mcbHeader = getNextMCB(mcbHeader.address, mcbHeader.mcb);
		printMCB(mcbHeader.mcb);
		if(((mcbHeader.mcb).name[0]=='$' )&&((mcbHeader.mcb).name[l]== '$'))
		{
			printf( "Hiding program: %s\n", (mcbHeader.mcb).name);
			hideApp(oldHeader,mcbHeader) ;
		}
		else
		{
			oldHeader = mcbHeader;
		}
	}
	return;
}/*end main()-------------------------------------*/
	
	
	
	
	
	
	
	
	
	