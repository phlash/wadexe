// Super hacky COM program that starts with 'IWAD' and some valid numbers that are a JMP..

// convince gas we're assembling 16bit (286) code
.text
.code16
.arch i286

/* WAD header:
	MAGIC 'IWAD'
	ENTRIES <int32>
	OFFSET <int32>
  12 bytes total.
*/

.global _start
_start:
.ascii "IWAD"
/* disassembles to:
	dec %cx
	push %di
	inc %cx
	inc %sp
  which we can work with.. but first a JMP to get out of WAD header into the empty space..
*/

	jmp _continue

/* JMP assembles to:
	0xeb 0x06 => 1771 WAD lumps
  we now declare the rest of the header (high 16-bits of ENTRIES, OFFSET, past our code)
*/
.byte 0
.byte 0
.int 0

_continue:
	// fix up after entry sequence
	dec %sp
	pop %di

	// run some C
	.extern cstart
	call cstart

	// bye-bye back to MS-DOS
	mov $0x4c, %ah
	mov $0, %al
	int $0x21

.end

