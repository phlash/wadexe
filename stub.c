// loader stub for DOOM - might unpack EXE from WAD (cheating)
// or re-map INT21h file reads into the WAD (proper)

#define CARRY 0x0001

// raw DOS API call
typedef struct {
	short ax;
	short bx;
	short cx;
	short dx;
	short di;
	short fl;
} reg_t;

// faucc seems to only work correctly when referrring to local vars,
// hence the copying in/out from the provided pointer...
short doscall(reg_t *pregs) {
	short ax = pregs->ax;
   	short bx = pregs->bx;
   	short cx = pregs->cx;
   	short dx = pregs->dx;
	short di = pregs->di;
   	short fl;
	asm(
		"mov %6, %%ax;"
		"mov %7, %%bx;"
		"mov %8, %%cx;"
		"mov %9, %%dx;"
		"mov %10, %%di;"
		"int $0x21;"
		"pushf;"
		"pop %0;"
		"mov %%ax, %1;"
		"mov %%bx, %2;"
		"mov %%cx, %3;"
		"mov %%dx, %4;"
		"mov %%di, %5;"
		: "=m"(fl), "=m"(ax), "=m"(bx), "=m"(cx), "=m"(dx), "=m"(di)
		: "m"(ax), "m"(bx), "m"(cx), "m"(dx), "m"(di)
		: "ax", "bx", "cx", "dx", "di"
	);
	pregs->ax = ax;
	pregs->bx = bx;
	pregs->cx = cx;
	pregs->dx = dx;
	pregs->di = di;
	pregs->fl = fl;
	return ax;
}

// Text output
int puts(char *str) {
	char *s = str;
	while (*s) {
		// AH=02h, write one char to stdout
		reg_t reg;
		reg.ax = 0x0200;
		reg.dx = (short)(*s);
		doscall(&reg);
		s++;
	}
	return s-str;
}

int putint(unsigned int val) {
	// recursion - yay!
	int rv = 1;
	reg_t reg;
	if (val>9)
		rv += putint(val/10);
	val = val%10;
	reg.ax = 0x0200;
	reg.dx = (short)(val+'0');
	doscall(&reg);
	return rv;
}

int puthex(unsigned int val) {
	int rv = 1;
	reg_t reg;
	if (val>0xf)
		rv += puthex(val/16);
	val=val%16;
	reg.ax = 0x0200;
	reg.dx = (short)(val<10?(val+'0'):(val-10+'A'));
	doscall(&reg);
	return rv;
}

int putregs(reg_t *pregs) {
	int rv = puts("[regs: ax=");
	rv += puthex(pregs->ax&0xffff);
	rv += puts(" bx=");
	rv += puthex(pregs->bx&0xffff);
	rv += puts(" cx=");
	rv += puthex(pregs->cx&0xffff);
	rv += puts(" dx=");
	rv += puthex(pregs->dx&0xffff);
	rv += puts(" fl=");
	rv += puthex(pregs->fl&0xffff);
	rv += puts("] ");
	return rv;
}

// File I/O
int open(char *path, int wr) {
	// AH=5Bh, open file DOS 2+ (create/trunc, compat mode)
	// AH=3Dh, open file DOS 2+ (read only, compat mode)
	int rv;
	reg_t reg;
	reg.ax = wr ? 0x5B00 : 0x3D00;
	reg.cx = 0;
	reg.dx = (short)path;
	rv = doscall(&reg);
	if (reg.fl & CARRY)
		return -(rv&0xffff);
	return (rv&0xffff);
}

int read(int fd, void *buf, int len) {
	// AH=3Fh, read file
	int rv;
	reg_t reg;
	reg.ax = 0x3F00;
	reg.bx = (short)fd;
	reg.cx = (short)len;
	reg.dx = (short)buf;
	rv = doscall(&reg);
	if (reg.fl & CARRY)
		return -(rv&0xffff);
	return (rv&0xffff);
}

int write(int fd, void *buf, int len) {
	// AH=40h, write file
	int rv;
	reg_t reg;
	reg.ax = 0x4000;
	reg.bx = (short)fd;
	reg.cx = (short)len;
	reg.dx = (short)buf;
	rv = doscall(&reg);
	if (reg.fl & CARRY)
		return -(rv&0xffff);
	return (rv&0xffff);
}

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
int seek(int fd, int off, short whence) {
	// AH=42h, seek file
	int rv;
	short olow = (short)(off&0xffff);
	short ohgh = (short)(off>>16);
	reg_t reg;
	reg.ax = 0x4200|whence;
	reg.bx = (short)fd;
	reg.cx = ohgh;
	reg.dx = olow;
	rv = doscall(&reg);
	if (reg.fl & CARRY)
		return -(rv&0xffff);
	return (rv&0x7fffffff);
}

int close(int fd) {
	// AH=3Eh, close handle
	int rv;
	reg_t reg;
	reg.ax = 0x3E00;
	reg.bx = (short)fd;
	rv = doscall(&reg);
	if (reg.fl & CARRY)
		return -(rv & 0xffff);
	return 0;
}

int rename(char *src, char *dst) {
	// AH=56h, rename file
	int rv;
	reg_t reg;
	reg.ax = 0x5600;
	reg.dx = (short)src;
	reg.di = (short)dst;
	rv = doscall(&reg);
	if (reg.fl & CARRY)
		return -(rv&0xffff);
	return 0;
}

struct {
	short env;
	int cmd;
	int fcb1;
	int fcb2;
	int stk;
	int ent;
} prm = {
	0, 0, 0, 0, 0, 0
};
extern void _edata();
int exec(char *prg) {
	int rv;
	reg_t reg;
	// AH=4Ah, resize block (ours, chop down to minimum)
	short seg = (short)_edata;
	seg = (seg+15)/16;
	puts("\r\nexec free: ");
	puthex(seg);
	reg.ax = 0x4A00;
	reg.bx = seg;
	rv = doscall(&reg);
	if (reg.fl & CARRY)
		return -(rv&0xffff);
	// AH=4Bh, load & exceute
	puts("\r\nexec load: ");
	puts(prg);
	reg.ax = 0x4B00;
	reg.bx = (short)&prm;
	reg.cx = 0;
	reg.dx = (short)prg;
	rv = doscall(&reg);
	if (reg.fl & CARRY)
		return -(rv&0xffff);
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

#define BUFSIZ	32766
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
		puthex(-n);
		return n;
	}
	// search lumps for matching name
	do {
		int i;
		if ((n=read(fd, &lump, sizeof(lump)))<0) {
			puts("\r\noops reading lumps: ");
			puthex(-n);
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
		puthex(-n);
		return n;
	}
	// open outfile
	of = open(outfile, 1);
	if (of<0) {
		puts("\r\noops opening: ");
		puts(outfile);
		puts(": ");
		puthex(-of);
		return of;
	}
	// copy blocks
	tot = 0;
	puts("\r\n");
	while (tot<lump.size) {
		int r = BUFSIZ;
		if (tot+BUFSIZ>lump.size)
			r = lump.size-tot;
		n = read(fd, lbuf, r);
		if (n!=r) {
			puts("\r\noops reading lump: ");
			puts(name);
			puts(": n=");
			putint(n);
			puts(" r=");
			putint(r);
			return n;
		}
		if ((n=write(of, lbuf, n))!=r) {
			puts("\r\noops writing lump: ");
			puts(outfile);
			puts(": n=");
			putint(n);
			puts(" r=");
			putint(r);
			return n;
		}
		tot += n;
		puts("\rtot=");
		putint(tot);
	}
	puts("\r\nwrote: ");
	putint(tot);
	// close up
	close(of);
	return 0;
}

// entry point from wadexe.asm
void cstart() {
	int fd, rv;
	char *wfile;
	wad_header_t wad;
	if (sizeof(int)!=4) {
		puts("Error: sizeof(int)!=4\r\n");
		return;
	}
	wfile = getwad();
	puts("Welcome to DOOM1WAD.COM!\r\n");
	puts("opening: ");
	puts(wfile);
	fd = open(wfile, 0);
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
	puts("\r\nrenaming: ");
	puts(wfile);
	puts("->DOOM1.WAD");
	rename(wfile, "DOOM1.WAD");
	puts("\r\nRunning D.EXE...");
	rv = exec("D.EXE");
	if (rv<0) {
		puts("\r\noops exec D.EXE: ");
		puthex(-rv);
	}
}

