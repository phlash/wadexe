# Build 16-bit fun app that executes as a DOS COM file, but is also
# a valid DOOM WAD :-)

all: wadexe.com

wadexe.com: wadexe.elf
	objcopy -O binary -j .text -j .rodata $< $@
#	dd if=tmp.bin of=$@ bs=256 skip=1
#	rm tmp.bin

wadexe.elf: wadhdr.o stub.o
	ld -m elf_i386 -Ttext=0x100 -o $@ $^

wadhdr.o: wadhdr.asm
	as --32 -c -o $@ $<

stub.o: stub.c
	gcc -m16 -fno-pic -c -o $@ $<

clean:
	rm -f wadexe.* *.o tmp.bin

test: wadexe.com
	dosbox ./wadexe.com
