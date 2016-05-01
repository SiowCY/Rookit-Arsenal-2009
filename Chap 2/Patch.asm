;+------------------------------------------------------------------------------+
;																				+
;																				+
; PATCH . asm (simple case)														+
;																				+
;																				+
;+------------------------------------------------------------------------------+
;Basic gameplan:
;	Patch first four bytes of tree.com
;		Old code : CMP SP, 3EFF (81 FC 3EFF)
;		New code : JMP [low byte][hi byte] NOP (E9 A2 26 90)
;
;		Existing binary ends at offset 26A4
;			This will become 27M when loaded into RAM (due to 100H .CCtl PSP)
;		The JMP above is a near jump, and it uses a 16-bit signed displacement
;		Distance to jump =
;			Start 0103 (IP at end of E9 A7 27)
;			End 27A5 (first instruction of patch)
;			-------------------------------------
;				26A2 is displacement to jump
;
;	Then we use a hex editor to paste all the code between the jumps
;		JMP SHORT _main - > JMP BX
;	Only need one fix-up (the address of the message bytes, see below)
;
;	See dissection of hex dump in the book

CSEG SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:CSEG, OS:CSEG, SS:CSEG
; Need raw binary, can comment out ORG directive
; ORG 100H

_here:
JMP SHORT _main 	; EB 29 (start copying here)
_message DB 'We just jumped to the end of Tree.com!', 0AH, 0DH, 24H

; entry point------------------------------------------------------
_main:

;This code below needs to be patched manually
;needed to set to manually to address 26A7+100(COM PSP) = 27A7
;Jump instruction takes up 2 bytes (starting at offset 27A5)
;Buffer start at offset 27A7
;MOV DX, OFFSET _message goes from (BA 0002) to (BA A727), note the byte reversal

MOV AH, 09H			;B4 09
MOV DX, OFFSET _message		;BA 0002
INT 21H			;CD 21

; [Return Code]------------------------------------------------------
CMP SP,3EFFH 		;81 FC 3EFF (code we supplanted with our jump)
MOV BX,0104H 		; BB 0104 (goto code following inserted jump)
JMP BX 				;FF E3
;---------------------------------------------------------

; we can ignore everything after this comment
MOV AX, 4C00H
INT 21H
; stack for .COM program---------------------------------------------------------
; PUBLIC _localStk
;_localStk DB 64 DUP(' J' )
CSEG ENDS
END _here