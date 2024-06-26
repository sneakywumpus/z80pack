;
;	  THIS PROGRAM IS A SKELETAL OUTLINE FOR A 128-BYTE PRIMARY
;	BOOTSTRAP FOR AUTOMATICALLY BOOTING TO UCSD PASCAL (TM).
;	  SET THE CORRECT ORIGIN FOR THIS PROGRAM FOR YOUR SYSTEM, SET
;	'MSIZE' FOR THE APPROPRIATE NUMBER OF KILOBYTES OF RAM MEMORY
;	FOR YOUR SYSTEM, SET THE APPROPRIATE PARAMETERS DESCRIBING YOUR
;	DISK ENVIRONMENT AND FINALLY WRITE A VERY LOW LEVEL DISK READ
;	ROUTINE TO ALLOW READING IN THE SECONDARY BOOTSTRAP AND YOUR
;	CBIOS OFF THE DISK AND INTO RAM.
;	  THE PROGRAM 'CPMBOOT' ON THE UCSD PASCAL DISTRIBUTION DISK WILL
;	THEN USE THIS PROGRAM AND YOUR CBIOS TO GENERATE AN AUTOMATICALLY
;	BOOTING UCSD PASCAL SYSTEM.
;
;	ADAPTED FOR IMSAI 8080, JANUARY 2017, UDO MUNK
;	USE 8080 INSTRUCTIONS ONLY
;
BOOT	EQU	8200H		; SECONDARY BOOTSTRAP LOADED HERE
MSIZE	EQU	54		; MEMORY SIZE FOR ASSEMBLY
BIAS	EQU	(MSIZE*1024)-01900H
CBIOS	EQU	1500H+BIAS	; ORIGIN POINT
SECNUM	EQU	16		; SECONDARY BOOTSTRAP IS 16 SECTORS LONG
SECSEC	EQU	3		; SECONDARY BOOTSTRAP ON THIS SECTOR
BIOSNUM	EQU	8		; CBIOS IS 8 SECTORS LONG
BIOSSEC	EQU	19		; CBIOS IS ON THIS SECTOR
;
FIF	EQU	80H		; FIF DISK DESCRIPTOR
;
;	IMSAI 8080 I/O PORTS
;
FDC	EQU	0FDH		; FDC PORT
;
	ORG	0		; WHATEVER IS RIGHT FOR YOUR SYSTEM
;
PBOOT:	LD	A,10H		; SETUP FDC DISK DESCRIPTOR
	OUT	(FDC),A
	LD	A,FIF & 0FFH
	OUT	(FDC),A
	LD	A,FIF >> 8
	OUT	(FDC),A
	LD	A,21H		; READ COMMAND UNIT 1
	LD	(FIF),A
	LD	HL,CBIOS	; CBIOS GOES HERE
	LD	SP,HL		; RESET THE STACK
	LD	D,BIOSNUM	; D - # OF SECTORS TO READ
	LD	E,BIOSSEC	; E - STARTING SECTOR
	CALL	READIT		; READ IN CBIOS
	LD	HL,BOOT		; LOAD BOOT BASE ADDRESS
	LD	D,SECNUM	; D - # OF SECTORS TO READ
	LD	E,SECSEC	; E - STARTING SECTOR
	CALL	READIT		; READ IN SECONDARY BOOTSTRAP
	LD	HL,128		; MAXIMUM NUMBER OF BYTES PER SECTOR
	PUSH	HL
	LD	HL,26		; MAXIMUM NUMBER OF SECTORS IN TABLE
	PUSH	HL
	LD	HL,0		; TRACK-TO-TRACK SKEW
	PUSH	HL
	LD	HL,1		; FIRST INTERLEAVED TRACK
	PUSH	HL
	LD	HL,1		; 1:1 INTERLEAVING
	PUSH	HL
	LD	HL,128		; BYTES PER SECTOR
	PUSH	HL
	LD	HL,26		; SECTORS PER TRACK
	PUSH	HL
	LD	HL,77		; TRACKS PER DISK
	PUSH	HL
	LD	HL,CBIOS-2	; TOP OF MEMORY (MUST BE WORD BOUNDARY)
	PUSH	HL
	LD	HL,0100H	; BOTTOM OF MEMORY
	PUSH	HL
	LD	DE,CBIOS+3	; START OF THE SBIOS (JMP WBOOT)
	PUSH	DE
	PUSH	HL		; STARTING ADDRESS OF INTERPRETER
	JP	BOOT		; ENTER SECONDARY BOOTSTRAP
;
;	  READIT MUST READ THE NUMBER OF SECTORS SPECIFIED IN THE D
;	REG, STARTING AT THE SECTOR SPECIFIED IN THE E REG, INTO THE
;	MEMORY LOCATION SPECIFIED IN THE HL PAIR.
;
READIT:	
;
;  PUT YOUR CODE IN HERE
;
L1:
	LD	A,E		; SELECT SECTOR
	LD	(FIF+4),A
	LD	A,L		; SET DMA ADDRESS LOW
	LD	(FIF+5),A
	LD	A,H		; SET DMA ADDRESS HIGH
	LD	(FIF+6),A
	XOR	A		; RESET RESULT
	LD	(FIF+1),A
	OUT	(FDC),A		; READ SECTOR
L1A:	LD	A,(FIF+1)	; WAIT FOR FDC
	OR	A
	JP	Z,L1A
	CP	1		; RESULT = 1 ?
	JP	Z,L2		; YES, CONTINUE
	HALT			; FAILURE, HALT CPU
L2:
	DEC	D		; SECTORS = SECTORS - 1
	RET	Z		; RETURN IF ALL SECTORS LOADED
	INC	E		; NEXT SECTOR TO READ
	LD	BC,128		; 128 BYTES PER SECTOR
	ADD	HL,BC		; DMA ADDRESS + 128
	JP	L1		; GO READ NEXT
;
	END	
