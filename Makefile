# Build 16-bit fun app that executes as a DOS COM file, but is also
# a valid DOOM WAD :-)
# NB: Using faucc as a workable 16-bit C compiler, with gas backend

all: wadexe.com wadinject

# faucc and our asm only emit .text and .data, but other tools (gcc)
# tend to include other garbage, so we skip those when converting to
# binary file
wadexe.com: wadexe.elf
	objcopy -O binary -j .text -j .data $< $@

# DOS COM files always start execution @ 0x100 offset in a random segment
# we also ensure we are working with i386 ELF output so the debugger can
# understand what's going on inside Dosbox..
wadexe.elf: wadhdr.o stub.o
	ld -m elf_i386 -Ttext=0x100 -o $@ $^

# We assemble to i386 ELF (--32), and generate a listing to ensure GAS is
# working as expected (not trying to be clever and insert 32bit prefixes!)
wadhdr.o: wadhdr.asm
	as --32 -c -o $@ $< -a=$@.s

# faucc can't pass through assembler options, so we do our own.. to produce
# a listing as per GAS above
stub.o: stub.c
	faucc -b i286 -fsizeof_int=4 -S -o stub.s $<
	as --32 -c -o $@ stub.s -a=$@.s
	rm stub.s

wadinject: wadinject.c
	gcc -o $@ $<

clean:
	rm -f wadinject wadexe.* *.o *.o.s

test: wadexe.com
	dosbox ./wadexe.com
