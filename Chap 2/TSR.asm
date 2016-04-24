; +----------------------------------------------------------------------+
; TSR.asm (Terminate and stay resident program)
; Description : Implements a TSR that handles two interrupts
; The first returns the location of a buffer
; The second hooks BIOS int 0x9 (system timer maskable hardware bp)
; +--------------------------------------------------------------------+

CSEG SEGMENT BYTE PUBLIC 'COOE'
ASSUME CS:CSEG, DS:CSEG, SS:CSEG
ORG 100H

; This label defines the starting point ( see END statement )-----------------------
_here:
JMP _main

; global data-----------------------------------------------------
JMP _overData
_buffer DB 512 DUP('W')
_terminator DB 'z'
_index DW DH
_oldISR DD DH
_chkISR DD DH
_over Data :

; ISR to return address of buffer -------------------------------------------
_getBufferAddr:
STI
MOV DX,CS
LEA DI,_buffer
IRET

; ISR to hook BIOS int 0x9-----------------------------------
_hookBIOS :
PUSH BX
PUSH AX

PUSHF		;far call to old BIOS routine
CALL CS :_oldISR

MOV AH,01H		;check DOS buffer
PUSHF
CALL CS:_chkISR

CLI
PUSH DS			;need to adjust OS to acces s data
PUSH CS
POP DS

jz _hb_Exit			;if ZF=l, buffer is empty (result from call to _chkISR)
LEA BX , _buffer
PUSH SI

MOV SI, WORD PTR [_index]
MOV BYTE PTR [BX+SI], AL
INC SI
MOV WORD PTR [_index], SI
POP SI

_hb_Exit:
POP DS
POP AX
POP BX

STI
IRET

;	INT 0x21, AH = 0x2S Set Interrupt Vector
;	AL=interrupt;
;	DS : DX=addres s of ISR
;	INT 0x21, AH = 0x35 Get an Interrupt Vector
;	AL=interrupt
;	ES: BX=address of ISR
;	AH function code 31H (make resident)
;	AL Return code
;	DX Size of memory to set aside (in 16-byte paragraphs)
;	1 KB = 64 paragraph (ex4e paragraphs)
;	Note: can verify install code via KDDS. exe

;	install the TSR------------------------------------------------
_install :
LEA DX,_getBufferAddr ; set up first ISR (Vector 187 = 0xBB)
MOV CX,CS
MOV DS,CX
MOV AH,25H
MOV AL,187
INT 21H

; get address of existing BIOS 0x9 interrupt
MOV AH,35H
MOV AL,09H
INT 21H
MOV WORD PTR _oldISR[0],BX
MOV WORD PTR _oldISR[2],ES

; get address of existing BIOS 0x16 interrupt
MOV AH,35H
MOV AL,16H
INT 21H
MOV WORD PTR _chkISR[0], BX
MOV WORD PTR _chkISR[2], ES

; set up BIOS ISR hook
LEA DX,_hookBIOS set up first ISR (Vector 187 = 0xBB)
MOV CX,CS
MOV DS,CX
MOV AH,25H
MOV AL,09H
INT 21H

RET

; entry point-----------------------------------------
PUBLIC _main
_main:
PUSH BP				; set up stack
MOV BP,SP
MOV AX,CS
MOV SS,AX
LEA AX, _localStk
ADD AX,100H
MOV SP ,AX

CALL NEAR PTR _install
.
;	DOS maintains a pointer to the start of free memory in conventional memory
;	Programs are loaded at this position
;	When a program terminates, the pointer typically returns to its old value
;	A TSR increments the pointer ' s value so that the TSR isn' t overwritten

MOV AH,31H			;make this program resident
MOV AL,0H
MOV DX,200H
INT 21H
POP BP
RET

; stack for .COM program---------------------------------------------------------
PUBLIC _localStk
_localStk DB 256 DUP(?)

CSEG ENDS
END _here
