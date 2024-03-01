#include "x86.h"
#include "device.h"

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

void GProtectFaultHandle(struct TrapFrame *tf);

void KeyboardHandle(struct TrapFrame *tf);

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallPrint(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallGetChar(struct TrapFrame *tf);
void syscallGetStr(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf)
{ // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds" ::"a"(KSEL(SEG_KDATA)));
	// asm volatile("movw %%ax, %%es"::"a"(KSEL(SEG_KDATA)));
	// asm volatile("movw %%ax, %%fs"::"a"(KSEL(SEG_KDATA)));
	// asm volatile("movw %%ax, %%gs"::"a"(KSEL(SEG_KDATA)));
	
	// unsigned int copy_entry = tf->irq;
	// while (copy_entry)  // test error code 
	// {
	// 	char c = copy_entry % 10 + '0';
	// 	putChar(c);
	// 	copy_entry /= 10;
	// }

	switch (tf->irq)
	{
	// TODO: 填好中断处理程序的调用
	case 0x21:
		KeyboardHandle(tf);
		break;
	case 0x80:
		syscallHandle(tf);
		break;
	case 0xd:
		putChar('?');
		GProtectFaultHandle(tf);
		break;
	default:
		putChar('!');
		assert(0);
	}
}

void GProtectFaultHandle(struct TrapFrame *tf)
{
	assert(0);
	return;
}


int volatile readOrnot = 0;
void KeyboardHandle(struct TrapFrame *tf)
{	
	if(!readOrnot) return;
	uint32_t code = getKeyCode();
	if (code == 0xe)
	{ // 退格符
		// TODO: 要求只能退格用户键盘输入的字符串，且最多退到当行行首
		if(bufferHead == bufferTail) {
			;
		}else {
			bufferTail = (bufferTail - 1) % MAX_KEYBUFFER_SIZE;
			if(displayCol == 0) {
				displayCol = 24;
				displayRow -= 1;
			}else {
				displayCol -= 1;
			}
			uint16_t data = 0 | (0x0c << 8);
			int pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
		}
		updateCursor(displayRow, displayCol);
	}
	else if (code == 0x1c)
	{ // 回车符
	  // TODO: 处理回车情况
		displayRow += 1;
		displayCol = 0;
		if(displayRow == 25) {
				displayRow = 24;
				scrollScreen();
		}
		readOrnot = 0;
		updateCursor(displayRow, displayCol);
	}
	else if (code < 0x81)
	{ // 正常字符
		// TODO: 注意输入的大小写的实现、不可打印字符的处理
		if((code != 0xf && code != 0x1d && code!=0x2a && code <= 0x35) || code == 0x39 ) { // readable char
			char character = getChar(code);
			keyBuffer[bufferTail] = character;
			bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;
			uint16_t data = character | (0x0c << 8);
			int pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
			displayCol += 1;
			if (displayCol >= 80)
			{
				displayRow += 1;
				displayCol = 0;
				if(displayRow == 25) {
					displayRow = 24;
					scrollScreen();
				}
			}
		}
		updateCursor(displayRow, displayCol);
	}
}

void syscallHandle(struct TrapFrame *tf)
{
	switch (tf->eax)
	{ // syscall number
	case 0:
		syscallWrite(tf);
		break; // for SYS_WRITE
	case 1:
		syscallRead(tf);
		break; // for SYS_READ
	default:
		break;
	}
}

void syscallWrite(struct TrapFrame *tf)
{
	switch (tf->ecx)
	{ // file descriptor
	case 0:
		syscallPrint(tf);
		break; // for STD_OUT
	default:
		break;
	}
}

void syscallPrint(struct TrapFrame *tf)
{
	int sel = KSEL(SEG_UDATA); // TODO: segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es" ::"m"(sel));
	for (i = 0; i < size; i++)
	{
		asm volatile("movb %%es:(%1), %0"
					 : "=r"(character)
					 : "r"(str + i));
		// TODO: 完成光标的维护和打印到显存
		data = character | (0x0c << 8);
		pos = (80 * displayRow + displayCol) * 2;
		if (character == '\n')
		{
			displayCol = 0;
			displayRow += 1;
			if(displayRow == 25) {
				displayRow = 24;
				scrollScreen();
			}
		}
		else
		{
			asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
			displayCol += 1;
			if (displayCol >= 80)
			{
				displayRow += 1;
				displayCol = 0;
				if(displayRow == 25) {
					displayRow = 24;
					scrollScreen();
				}
			}
		}
		updateCursor(displayRow, displayCol);
	}
	tf->eax = size;
	updateCursor(displayRow, displayCol);
}

void syscallRead(struct TrapFrame *tf)
{
	enableInterrupt();
	switch (tf->ecx)
	{ // file descriptor
	case 0:
		syscallGetChar(tf);
		break; // for STD_IN
	case 1:
		syscallGetStr(tf);
		break; // for STD_STR
	default:
		break;
	}
}

void syscallGetChar(struct TrapFrame *tf)
{
	// TODO: 自由实现
	readOrnot = 1;
	while(readOrnot) ;
	char c = keyBuffer[(bufferTail-1)%MAX_KEYBUFFER_SIZE];
	bufferHead = 0;
	bufferTail = 0;
			
	tf->eax = c;
}

void syscallGetStr(struct TrapFrame *tf)
{
	// TODO: 自由实现
	readOrnot = 1;
	while(readOrnot) ;
	int sel = KSEL(SEG_UDATA);
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	char c;
	asm volatile("movw %0, %%es" ::"m"(sel));
	for (int i = 0; i < size; i++)
	{
		c = keyBuffer[(bufferHead+i)%MAX_KEYBUFFER_SIZE];
		asm volatile("movb %1, %%es:(%0)" ::"r"(str + i), "r"(c));
		if(c == '\n') break;
	}
	bufferHead = 0;
	bufferTail = 0;
}
