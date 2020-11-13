// Super hacky COM program that starts with 'IWAD' and some valid numbers that are a JMP..

.global _start
.org 0x100
.code16

/* WAD header:
	MAGIC 'IWAD'
	ENTRIES <int32>
	OFFSET <int32>
  12 bytes total.
*/

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
.int _wadinfotab

_continue:
	// fix up after entry sequence
	dec %sp
	pop %di
	// say hi!
	mov $_himum, %dx
	mov $9, %ah
	int $0x21

	// bye-bye back to MS-DOS
	mov $0x4c00, %ax
	int $0x21

_himum:
.ascii "Hi Mum!$"

_wadinfotab:
// here we prefix the info table from the source WAD with null entries to reach 1771,
// then simply concatenate the remaining real entries, fixing up any offsets
// - tada, valid WAD ;)
.end

