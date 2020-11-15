// Inject the COM program into a WAD file..

#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

int process(char *wadfile, char *outfile) {
	// for now - we dump WAD info
	int rv=1;
	FILE *wad = fopen(wadfile, "rb");
	wad_header_t hdr;
	if (!wad) {
		perror("opening wadfile");
		goto oops;
	}
	if (fread(&hdr, sizeof(hdr), 1, wad)!=1) {
		perror("reading header");
		goto oops;
	}
	printf("type: %.4s count: %u offset: %x\n",
		hdr.ident, hdr.count, hdr.offset);
	if (strncmp(hdr.ident, "IWAD", 4)!=0) {
		fprintf(stderr, "not an IWAD\n");
		goto oops;
	}
	if (fseek(wad, hdr.offset, SEEK_SET)<0) {
		perror("seeking to directory");
		goto oops;
	}
	for (int lidx=0; lidx<hdr.count; lidx++) {
		lump_t lump;
		if (fread(&lump, sizeof(lump), 1, wad)!=1) {
			perror("reading lump info");
			goto oops;
		}
		printf("%04d: offset: %08x size: %06u name: %.8s\n",
			lidx, lump.offset, lump.size, lump.name);
	}
	rv = 0;
oops:
	if (wad)
		fclose(wad);
	return rv;
}

int usage() {
	puts("usage: wadinject [-h] [-w <wadfile>] [-o <outfile>]");
	return 0;
}

int main(int argc, char **argv) {
	char *wadfile = "doom19s/DOOM1.WAD";
	char *outfile = "DOOM1EXE.COM";
	for (int arg=1; arg<argc; arg++) {
		if (!strncmp(argv[arg],"-h",2) || !strncmp(argv[arg],"--h",3))
			return usage();
		else if (!strncmp(argv[arg],"-w",2))
			wadfile = argv[++arg];
		else if (!strncmp(argv[arg],"-o",2))
			outfile = argv[++arg];
		else
			printf("ignoring unknown arg: %s\n", argv[arg]);
	}
	printf("wadinject: wadfile=%s, outfile=%s\n", wadfile, outfile);
	return process(wadfile, outfile);
}

