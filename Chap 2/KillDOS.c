/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+ 																				+
+ KOOS.C 																		+
+ 																				+
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include<stdio.h>
#define WORD unsigned short
#define IDT_001_ADDR 0 			//start address of first IVT vector
#define IDT_255_ADDR 1020 		//start address of last IVT vector
#define IDT_VECTOR_SZ 4 		//size of each IVT Vector (in bytes)
#define BP 	_asm{ int 0x3 } 	// break point
void main()
{
	WORD csAddr;		// Code segment of given interrupt
	WORD ipAddr;		//Starting IP for given interrupt
	short address;		//address in memory (0-1020)
	WORD vector ;		//IVT entry ID (ie. , 0.. 255)
	char dummy;			//strictly to help pause program execution
	
	vector = 0x0;
		
	printf(" \n---Dumping IVT from bottom up---\n");
	printf( "Vector\tAddress\t\n");
	for
	(
		address=IDT_001_ADDR;
		address<=IDT_255_ADDR;
		address=address+IDT_VECTOR_SZ,vector++
	)
	{
		printf("%e3d\t%e8p\t", vector, address);
		
		// IVT starts at bottom of memory, so CS is always 0x0
		
		_asm
		{
			PUSH ES
			MOV AX,0
			MOV ES,AX
			MOV BX, address
			MOV AX, ES: [BX]
			MOV ipAddr,AX
			INC BX
			INC BX
			MOV AX, ES: [BX]
			MOV csAddr, AX
			POP ES
		};
		printf("[CS: IP]=[%04X,%04X]\n" ,csAddr, ipAddr);
	}
	printf("press [ENTER] key to continue:");
	scanf( "%c ", &dummy) ;
	
	printf("\n---Overwrite IVT from top down---\n");
/*
Program will die somewhere around ex4*
Note: can get same results via 005 debug. exe -e corrrnand
*/
for
(
	address = IDT_255_ADDRR;
	address >= IDT_001l_ADDR;
	address = address-IDT_VECTOR_SZ,vector--
)
{
	printf( "Nulling %e3d\t%eBp\n", vector, address);
	_asm
	{
		PUSH ES
		MOV AX,0
		MOV ES,AX
		MOV BX, address
		MOV ES: [BX],AX
		INC BX
		INC BX
		rov ES: [BX], AX
		POP ES
	};
}
return;
}/*end main()----------------------------------------------*/
