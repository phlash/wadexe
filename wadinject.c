// Inject the COM program into a WAD file..

#include <stdio.h>
#include <stdint.h>
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

int process(char *wadfile, char *outfile, char *wadexe) {
	int rv=1, lcnt, rd, idx, esz, isz;
	wad_header_t hdr;
	wad_header_t *exe;
	lump_t *wadlumps;
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
	if (hdr.count >= lcnt) {
		fprintf(stderr, "%s: too many lumps to inject\n", wadfile);
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
	// calculate size of injected COM program
	isz = esz-sizeof(wad_header_t);
	// adjust offset to directory in outfile
	// assumes directory was at end of wadfile!
	exe->offset = hdr.offset + isz;
	// write outfile header + COM program
	if (fwrite(exe, esz, 1, out)!=1) {
		perror("writing outfile");
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
	wadlumps[idx].size = isz;
	strcpy(wadlumps[idx].name, "COMPROG");
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

int usage() {
	puts("usage: wadinject [-h] [-w <wadfile>] [-e <wadexe.com>] [-o <outfile>]");
	return 0;
}

int main(int argc, char **argv) {
	char *wadfile = "doom19s/DOOM1.WAD";
	char *wadexe  = "wadexe.com";
	char *outfile = "DOOM1EXE.COM";
	for (int arg=1; arg<argc; arg++) {
		if (!strncmp(argv[arg],"-h",2) || !strncmp(argv[arg],"--h",3))
			return usage();
		else if (!strncmp(argv[arg],"-w",2))
			wadfile = argv[++arg];
		else if (!strncmp(argv[arg],"-e",2))
			wadexe = argv[++arg];
		else if (!strncmp(argv[arg],"-o",2))
			outfile = argv[++arg];
		else
			printf("ignoring unknown arg: %s\n", argv[arg]);
	}
	printf("wadinject: wadfile=%s, eadexe=%s, outfile=%s\n", wadfile, wadexe, outfile);
	return process(wadfile, outfile, wadexe);
}

