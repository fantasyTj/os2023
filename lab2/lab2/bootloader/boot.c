#include "boot.h"

#define SECTSIZE 512

/*
void bootMain(void) {
	int i = 0;
	void (*elf)(void);
	elf = (void(*)(void))0x100000; // kernel is loaded to location 0x100000
	for (i = 0; i < 200; i ++) {
		//readSect((void*)elf + i*512, i+1);
		readSect((void*)elf + i*512, i+9);
	}
	elf(); // jumping to the loaded program
}
*/

void bootMain(void)
{
	int i = 0;
	int phoff = 0x34;
	int offset = 0x1000;
	unsigned int elf = 0x100000;
	void (*kMainEntry)(void);
	kMainEntry = (void (*)(void))0x100000;

	for (i = 0; i < 200; i++)
	{
		readSect((void *)(elf + i * 512), 1 + i);
	}

	// TODO: 填写kMainEntry、phoff、offset
	struct ELFHeader *eh = (struct ELFHeader *)elf;
	unsigned int entry = eh->entry;
	phoff = eh->phoff;
	struct ProgramHeader *ph = (struct ProgramHeader *)(elf + phoff);
	offset = ph->off;

	kMainEntry = (void (*)(void))(entry);

	// char c = 'a';
	// unsigned short data = c | (0x0c << 8);
	// asm volatile("movw %0, (%1)" ::"r"(data), "r"(0xb8000));

	for (i = 0; i < 200 * 512; i++)
	{
		*(unsigned char *)(elf + i) = *(unsigned char *)(elf + i + offset);
	}
	// while (1);
	kMainEntry();
}

void waitDisk(void)
{ // waiting for disk
	while ((inByte(0x1F7) & 0xC0) != 0x40)
		;
}

void readSect(void *dst, int offset)
{ // reading a sector of disk
	int i;
	waitDisk();
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);

	waitDisk();
	for (i = 0; i < SECTSIZE / 4; i++)
	{
		((int *)dst)[i] = inLong(0x1F0);
	}
}
