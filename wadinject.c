// Inject the COM loader and unwrapped LE/LX DOOM binary into a WAD file..

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

typedef struct {
	char ident[4];
	uint32_t count;
	uint32_t offset;
} wad_header_t;

typedef struct {
	uint32_t offset;
	uint32_t size;
	char name[8];
} lump_t;

void *loaddoom(char *doomexe, int *plen) {
	// Suck in the whole file
	FILE *dfp = fopen(doomexe, "rb");
	void *pdm;
	if (!dfp) {
		perror("opening doomexe");
		return NULL;
	}
	if (fseek(dfp, 0, SEEK_END)<0) {
		perror("seeking doomexe");
		return NULL;
	}
	*plen = ftell(dfp);
	fseek(dfp, 0, SEEK_SET);
	pdm = malloc(*plen);
	if (fread(pdm, *plen, 1, dfp)!=1) {
		perror("reading doomexe");
		return NULL;
	}
	fclose(dfp);
	return pdm;
}

void *loaddoomlx(char *doomexe, int *plen) {
	// Open DOS4/GW embedded binary
	FILE *dfp = fopen(doomexe, "rb");
	void *plx = NULL;
	char buf[64];
	int fnd, nrd;
	if (!dfp) {
		perror("opening doomexe");
		return NULL;
	}
	// search through multi-exec headers until we find LE/LX marker, taken from DOS32/A logic..
	// https://github.com/open-watcom/open-watcom-v2/blob/master/contrib/extender/dos32a/src/sb/sbind.asm#424
	fnd=0;
	while (!fnd) {
		int off;
		// read (next) exec header
		if (fread(buf, sizeof(buf), 1, dfp)!=1) {
			perror("reading doomexe");
			goto oops;
		}
		// skip MZ or BW exec..
		if (memcmp(buf, "MZ", 2)==0 ||
			memcmp(buf, "BW", 2)==0) {
			// calculate next exec offset and seek
			off = *((short*)(buf+2)) + *((short*)(buf+4))*512 - sizeof(buf);
			if (memcmp(buf, "MZ", 2)==0)
				off -= 512;
			if (fseek(dfp, off, SEEK_CUR)<0) {
				perror("seeking doomexe");
				goto oops;
			}
			continue;
		}
		// could this be the one?
		if (memcmp(buf, "LX\0\0", 4)==0 ||
			memcmp(buf, "LE\0\0", 4)==0) {
			fnd=1;
			break;
		}
		// move a byte, try again..
		if (fseek(dfp, 1-sizeof(buf), SEEK_CUR)<0) {
			perror("seeking doomexe");
			goto oops;
		}
	}
	if (!fnd) {
		fprintf(stderr, "failed to find LE/LX binary in doomexe\n");
		goto oops;
	}
	// read remainder of file into buffer
	fnd = ftell(dfp)-sizeof(buf);
	fseek(dfp, 0, SEEK_END);
	*plen = ftell(dfp)-fnd;
	fseek(dfp, fnd, SEEK_SET);
	plx = malloc(*plen);
	if (fread(plx, *plen, 1, dfp)!=1) {
		perror("reading doomexe");
		free(plx);
		plx = NULL;
	}
oops:
	if (dfp)
		fclose(dfp);
	return plx;
}

int process(char *wadfile, char *doomexe, char *outfile, char *wadexe) {
	int rv=1, lcnt, rd, idx, dsz, esz, isz;
	wad_header_t hdr;
	wad_header_t *exe;
	lump_t *wadlumps;
	void *doom;
	char *buf[BUFSIZ];
	// open files..
	FILE *wad = fopen(wadfile, "rb");
	FILE *out = fopen(outfile, "wb");
	FILE *fex = fopen(wadexe, "rb");
	if (!wad) {
		perror("opening wadfile");
		goto oops;
	}
	if (!out) {
		perror("opening outfile");
		goto oops;
	}
	if (!fex) {
		perror("opening wadexe");
		goto oops;
	}
	if (fread(&hdr, sizeof(hdr), 1, fex)!=1) {
		perror("reading wadexe header");
		goto oops;
	}
	lcnt = hdr.count;		// probably 1771 :)
	if (fread(&hdr, sizeof(hdr), 1, wad)!=1) {
		perror("reading wadfile header");
		goto oops;
	}
	// check wadfile is below lcnt
	if (hdr.count > lcnt-2) {
		fprintf(stderr, "%s: too many lumps to inject\n", wadfile);
		goto oops;
	}
	// load DOOM LE/LX binary (retaining embedded DOS4/GW)
	doom = loaddoom(doomexe, &dsz);
	if (!doom) {
		goto oops;
	}
	// load wadfile directory..
	printf("%s: type: %.4s count: %u offset: %x\n",
		wadfile, hdr.ident, hdr.count, hdr.offset);
	if (strncmp(hdr.ident, "IWAD", 4)!=0) {
		fprintf(stderr, "not an IWAD\n");
		goto oops;
	}
	if (fseek(wad, hdr.offset, SEEK_SET)<0) {
		perror("seeking to directory");
		goto oops;
	}
	wadlumps = alloca(sizeof(lump_t)*lcnt); // space for outfile lumps
	if (fread(wadlumps, sizeof(lump_t), hdr.count, wad)!=hdr.count) {
		perror("reading directory");
		goto oops;
	}
	// load wadexe..
	if (fseek(fex, 0, SEEK_END)<0) {
		perror("seeking wadexe");
		goto oops;
	}
	esz = ftell(fex);
	fseek(fex, 0, SEEK_SET);
	exe = alloca(esz);
	if (fread(exe, esz, 1, fex)!=1) {
		perror("reading wadexe");
		goto oops;
	}
	// calculate size of injected COM program+DOOM binary
	isz = esz+dsz-sizeof(wad_header_t);
	// adjust offset to directory in outfile
	// NB: assumes directory was at end of wadfile!
	exe->offset = hdr.offset + isz;
	// write outfile header + COM program
	if (fwrite(exe, esz, 1, out)!=1) {
		perror("writing outfile");
		goto oops;
	}
	// write DOOM binary
	if (fwrite(doom, dsz, 1, out)!=1) {
		perror("writing DOOM binary");
		goto oops;
	}
	// copy rest of wadfile content up to directory
	if (fseek(wad, sizeof(wad_header_t), SEEK_SET)<0) {
		perror("seeking wadfile");
		goto oops;
	}
	for (idx=sizeof(wad_header_t); idx<hdr.offset; idx+=rd) {
		// whole buf or partial?
		if (idx > hdr.offset-sizeof(buf))
			rd = hdr.offset-idx;
		else
			rd = sizeof(buf);
		if ((rd=fread(buf, 1, rd, wad))<0) {
			perror("reading wadfile");
			goto oops;
		}
		if (fwrite(buf, 1, rd, out)<0) {
			perror("writing outfile");
			goto oops;
		}
	}
	// adjust offsets in wadfile dictionary
	for (idx=0; idx<hdr.count; idx++) {
		wadlumps[idx].offset += isz;
	}
	// add new entry for COM program ;)
	wadlumps[idx].offset = sizeof(wad_header_t);
	wadlumps[idx].size = esz-sizeof(wad_header_t);
	strcpy(wadlumps[idx].name, "COMPROG");
	idx++;
	// add new entry for DOOM binary ;)
	wadlumps[idx].offset = esz;
	wadlumps[idx].size = dsz;
	strcpy(wadlumps[idx].name, "DOOMEXE");
	idx++;
	// add additional zero length entries to pad out to lcnt
	while (idx<lcnt) {
		wadlumps[idx].offset = 0;
		wadlumps[idx].size = 0;
		strcpy(wadlumps[idx].name, "PADDING");
		idx++;
	}
	// write modified directory to outfile
	if (fwrite(wadlumps, sizeof(lump_t), lcnt, out)!=lcnt) {
		perror("writing outfile");
		goto oops;
	}
	// success!
	rv = 0;
oops:
	if (fex)
		fclose(fex);
	if (wad)
		fclose(wad);
	if (out)
		fclose(out);
	return rv;
}

int viewwad(char *wadfile) {
	FILE *wad = fopen(wadfile, "rb");
	wad_header_t hdr;
	lump_t lump;
	int rv = 1;
	if (!wad) {
		perror("opening wadfile");
		goto oops;
	}
	if (fread(&hdr, sizeof(hdr), 1, wad)!=1) {
		perror("reading wadfile");
		goto oops;
	}
	printf("%s: type: %.4s, lumps: %d directory@%08x\n",
		wadfile, hdr.ident, hdr.count, hdr.offset);
	if (fseek(wad, hdr.offset, SEEK_SET)<0) {
		perror("seeking wadfile");
		goto oops;
	}
	for (int h=0; h<hdr.count; h++) {
		if (fread(&lump, sizeof(lump), 1, wad)!=1) {
			perror("reading lump descriptor");
			goto oops;
		}
		printf("  %04d: name: %8.8s, size: %6d, offset: %08x\n",
			h, lump.name, lump.size, lump.offset);
	}
	rv = 0;
oops:
	if (wad)
		fclose(wad);
	return rv;
}

int usage() {
	puts("usage: wadinject [-h] [-v] [-w <wadfile>] [-d <doom.exe>] [-e <wadexe.com>] [-o <outfile>]");
	return 0;
}

int main(int argc, char **argv) {
	char *wadfile = "doom19s/DOOM1.WAD";
	char *doomexe = "doom19s/DOOM.EXE";
	char *wadexe  = "wadexe.com";
	char *outfile = "DOOM1WAD.COM";
	int view = 0;
	for (int arg=1; arg<argc; arg++) {
		if (!strncmp(argv[arg],"-h",2) || !strncmp(argv[arg],"--h",3))
			return usage();
		else if (!strncmp(argv[arg],"-w",2))
			wadfile = argv[++arg];
		else if (!strncmp(argv[arg],"-d",2))
			doomexe = argv[++arg];
		else if (!strncmp(argv[arg],"-e",2))
			wadexe = argv[++arg];
		else if (!strncmp(argv[arg],"-o",2))
			outfile = argv[++arg];
		else if (!strncmp(argv[arg],"-v",2))
			view = 1;
		else
			printf("ignoring unknown arg: %s\n", argv[arg]);
	}
	if (view) {
		printf("wadinject: viewing: %s\n", wadfile);
		return viewwad(wadfile);
	} else {
		printf("wadinject: wadfile=%s, wadexe=%s, outfile=%s\n", wadfile, wadexe, outfile);
		return process(wadfile, doomexe, outfile, wadexe);
	}
}

