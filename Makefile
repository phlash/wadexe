# Build 16-bit fun app that executes as a DOS COM file, but is also
# a valid DOOM WAD :-)

wadexe.com: wadexe.elf
	objcopy -O binary $< tmp.bin
	dd if=tmp.bin of=$@ bs=256 skip=1
	rm tmp.bin

wadexe.elf: wadexe.asm
	as --32 -o $@ $<

clean:
	rm -f wadexe.com wadexe.elf tmp.bin

test: wadexe.com
	dosbox ./wadexe.com
