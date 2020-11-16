// loader stub for DOOM - might unpack EXE from WAD (cheating)
// or re-map INT21h file reads into the WAD (proper)

#define CARRY 0x010000

int doscall(short ax, short bx, short cx, short dx) {
	short fl;
	asm(
		"mov %2, %%ax;"
		"mov %3, %%bx;"
		"mov %4, %%cx;"
		"mov %5, %%dx;"
		"int $0x21;"
		"pushf;"
		"pop %0;"
		"mov %%ax, %1;"
		: "=m"(fl), "=m"(ax)
		: "m"(ax), "m"(bx), "m"(cx), "m"(dx)
		:
	);
	return ((int)fl<<16)|(int)ax;
}

int open(char *path) {
	// AH=3Dh, open file DOS 2+ (read only, compat mode)
	int rv = doscall(0x3D00, 0, 0, (short)path);
	if (rv & CARRY)
		return -(rv&0xffff);
	return (rv&0xffff);
}

int read(int fd, void *buf, int len) {
	// AH=3Fh, read file
	int rv = doscall(0x3F00, (short)fd, (short)len, (short)buf);
	if (rv & CARRY)
		return -(rv&0xffff);
	return (rv&0xffff);
}

int close(int fd) {
	// AH=3Eh, close handle
	int rv = doscall(0x3E00, (short)fd, 0, 0);
	if (rv & CARRY)
		return -(rv & 0xffff);
	return 0;
}

int puts(char *str) {
	char *s = str;
	while (*s) {
		// AH=02h, write one char to stdout
		doscall(0x0200, 0, 0, (short)(*s));
		s++;
	}
	return s-str;
}

char *getwad() {
	// check command line, default to DOOM1EXE.COM
	unsigned char l = *((unsigned char *)0x80);
	char *c = (char *)0x81;
	char *r = 0;
	unsigned char i;
	if (!l)
		return "DOOM1EXE.COM";
	for (i=0; i<l; i++) {
		if (!r && c[i]>' ')
			r = c+i;
	}
	c[i] = 0;
	return r;
}

void cstart() {
	char buf[12];
	int fd;
	if (sizeof(int)!=4) {
		puts("Error: sizeof(int)!=4\r\n");
		return;
	}
	fd = open(getwad());
	if (fd<0) {
		puts("\r\noops opening file");
		return;
	}
	if (read(fd, buf, 12)!=12) {
		puts("\r\noops reading WAD header");
		close(fd);
		return;
	}
	close(fd);
	buf[5]=0;
	puts(buf);
	puts("\r\nSuccess!\r\n");
}

