// loader stub for DOOM - might unpack EXE from WAD (cheating)
// or re-map INT21h file reads into the WAD (proper)

void cstart() {
	char *yo = "\r\nIt's C time!$";	// Yes, a DOS string ;)
	asm(
		"mov %0, %%edx;"
		"mov $0x09, %%ah;"
		"int $0x21"
		: // no outputs
		: "m" (yo)
		: "edx", "eax"
	);
}

