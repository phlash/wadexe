// loader stub for DOOM - might unpack EXE from WAD (cheating)
// or re-map INT21h file reads into the WAD (proper)

#define CARRY 0x010000

// raw DOS API call
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

// Text output
int puts(char *str) {
	char *s = str;
	while (*s) {
		// AH=02h, write one char to stdout
		doscall(0x0200, 0, 0, (short)(*s));
		s++;
	}
	return s-str;
}

int putint(unsigned int val) {
	// recursion - yay!
	int rv = 1;
	if (val>9)
		rv += putint(val/10);
	val = val%10;
	doscall(0x0200, 0, 0, (short)(val+'0'));
	return rv;
}

int puthex(int val) {
	int rv = 1;
	if (val>0xf)
		rv += puthex(val/16);
	val=val%16;
	doscall(0x0200, 0, 0, (short)(val<10?(val+'0'):(val-10+'A')));
	return rv;
}

// File I/O
int open(char *path, char wr) {
	// AH=3Ch, open file DOS 2+ (create/trunc, compat mode)
	// AH=3Dh, open file DOS 2+ (read only, compat mode)
	int rv = doscall(wr?0x3C00:0x3D00, 0, 0, (short)path);
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

int write(int fd, void *buf, int len) {
	// AH=40h, write file
	int rv = doscall(0x4000, (short)fd, (short)len, (short)buf);
	if (rv & CARRY)
		return -(rv&0xffff);
	return (rv&0xffff);
}

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
int seek(int fd, int off, short whence) {
	// AH=42h, seek file
	short olow = (short)(off&0xffff);
	short ohgh = (short)(off>>16);
	int rv = doscall(0x4200|whence, (short)fd, ohgh, olow);
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

// command line override
char *getwad() {
	// check command line, default to DOOM1WAD.COM
	unsigned char l = *((unsigned char *)0x80);
	char *c = (char *)0x81;
	char *r = 0;
	unsigned char i;
	if (!l)
		return "DOOM1WAD.COM";
	for (i=0; i<l; i++) {
		if (!r && c[i]>' ')
			r = c+i;
	}
	c[i] = 0;
	return r;
}

// WAD processing
typedef struct {
	char ident[4];
	int count;
	int offset;
} wad_header_t;

typedef struct {
	int offset;
	int size;
	char name[8];
} lump_t;

#define BUFSIZ	4096
static char lbuf[BUFSIZ];
int unpack(int fd, wad_header_t *pwad, char *name, char *outfile) {
	lump_t lump;
	int of, tot, n;
	char fnd=0;
	// seek to directory
	puts("\r\ndir seek: ");
	puthex(pwad->offset);
	if ((n=seek(fd, pwad->offset, SEEK_SET))<0) {
		puts("\r\noops seeking WAD: ");
		puthex(n);
		return n;
	}
	// search lumps for matching name
	do {
		int i;
		if ((n=read(fd, &lump, sizeof(lump)))<0) {
			puts("\r\noops reading lumps: ");
			puthex(n);
			return n;
		}
		fnd=1;
		for (i=0; name[i]; i++) {
			if (lump.name[i]!=name[i])
				fnd=0;
		}
	} while (!fnd);
	if (!fnd) {
		puts("\r\nNo such lump: ");
		puts(name);
		return -1;
	}
	// seek to lump
	puts("\r\nlump seek: ");
	puthex(lump.offset);
	if ((n=seek(fd, lump.offset, SEEK_SET))<0) {
		puts("\r\noops seeking to lump: ");
		puthex(n);
		return n;
	}
	// open outfile
	of = open(outfile, 1);
	if (of<0) {
		puts("\r\noops opening: ");
		puts(outfile);
		puts(": ");
		puthex(of);
		return of;
	}
	// copy blocks
	tot = 0;
	while (tot<lump.size) {
		int r = BUFSIZ;
		if (tot+BUFSIZ>lump.size)
			r = lump.size-tot;
		n = read(fd, lbuf, r);
		if (n<=0) {
			puts("\r\noops reading lump: ");
			puts(name);
			puts(": ");
			puthex(n);
			return n;
		}
		if ((n=write(of, lbuf, n))<0) {
			puts("\r\noops writing: ");
			puts(outfile);
			puts(": ");
			puthex(n);
			return n;
		}
		tot += n;
	}
	puts("\r\nwrote: ");
	putint(tot);
	// close up
	close(of);
	return 0;
}

// entry point from wadexe.asm
void cstart() {
	int fd;
	wad_header_t wad;
	if (sizeof(int)!=4) {
		puts("Error: sizeof(int)!=4\r\n");
		return;
	}
	fd = open(getwad(), 0);
	if (fd<0) {
		puts("\r\noops opening file");
		return;
	}
	if (read(fd, &wad, sizeof(wad))!=sizeof(wad)) {
		puts("\r\noops reading WAD header");
		close(fd);
		return;
	}
	puts("\r\nUnpacking DOOM.EXE->D.EXE");
	if (unpack(fd, &wad, "DOOMEXE", "D.EXE")<0) {
		puts("\r\noops unpacking DOOM");
		close(fd);
		return;
	}
	close(fd);
	puts("\r\nRunning D.EXE...");
}

