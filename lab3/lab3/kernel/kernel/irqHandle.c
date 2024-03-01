#include "device.h"
#include "x86.h"

extern int displayRow;
extern int displayCol;

extern ProcessTable pcb[MAX_PCB_NUM]; // pcb
extern int current; // current process
extern TSS tss;

void GProtectFaultHandle(struct StackFrame *sf);

void syscallHandle(struct StackFrame *sf);

void syscallWrite(struct StackFrame *sf);
void syscallPrint(struct StackFrame *sf);
void timerHandle(struct StackFrame *sf);
void syscallFork(struct StackFrame *sf);
void syscallSleep(struct StackFrame *sf);
void syscallExit(struct StackFrame *sf);

void irqHandle(struct StackFrame *sf) {  // pointer sf = esp
    /* Reassign segment register */
    asm volatile("movw %%ax, %%ds" ::"a"(KSEL(SEG_KDATA)));
    /* Save esp to stackTop */
    uint32_t tmpStackTop = pcb[current].stackTop;
    pcb[current].prevStackTop = pcb[current].stackTop;
    pcb[current].stackTop = (uint32_t)sf;

    switch (sf->irq) {
        case -1:
            break;
        case 0xd:
            GProtectFaultHandle(sf);
            break;
        case 0x20:
            timerHandle(sf);
            break;
        case 0x80:
            syscallHandle(sf);
            break;
        default:
            assert(0);
    }

    /* Recover stackTop */
    pcb[current].stackTop = tmpStackTop;
}

void GProtectFaultHandle(struct StackFrame *sf) {
    assert(0);
    return;
}

void syscallHandle(struct StackFrame *sf) {
    switch (sf->eax) {  // syscall number
        case 0:
            syscallWrite(sf);
            break;  // for SYS_WRITE (0)
        /*TODO Add Fork,Sleep... */
        case 1:
            syscallFork(sf);
            break;
        case 3:
            syscallSleep(sf);
            break;
        case 4:
            syscallExit(sf);
            break;
        default:
            break;
    }
}

void timerHandle(struct StackFrame *sf) {
    // TODO in lab3
    for(int i = 1; i < MAX_PCB_NUM; i++) {
        if(pcb[i].state == STATE_DEAD) continue;
        else if(pcb[i].state == STATE_BLOCKED) {
            pcb[i].sleepTime -= 1;
            if(pcb[i].sleepTime == 0) pcb[i].state = STATE_RUNNABLE;
        }
    }

    if(pcb[current].state == STATE_RUNNING) {
        if(pcb[current].timeCount != MAX_TIME_COUNT) {
            pcb[current].timeCount += 1;
            return;
        }else{
            pcb[current].timeCount = 0;
            pcb[current].state = STATE_RUNNABLE;
        }
    }
    int check = (current + 1) % 4;
    int flag = 0;
    for(int i = 0; i < 3; i++) {
        if((pcb[check].state == STATE_RUNNABLE) && (check != 0)) {
            flag = 1;
            break;
        }else check = (check + 1) % 4;
    }
    if(flag == 1) current = check;
    else current = 0;
    // putChar('T');
    // putChar('0'+current);

    uint32_t tmpStackTop = pcb[current].stackTop;
    pcb[current].stackTop = pcb[current].prevStackTop;
    tss.esp0 = (uint32_t)&(pcb[current].stackTop);
    asm volatile("movl %0, %%esp" ::"m"(tmpStackTop));
    asm volatile("popl %gs");
    asm volatile("popl %fs");
    asm volatile("popl %es");
	asm volatile("popl %ds");
	asm volatile("popal");
	asm volatile("addl $8, %esp");
	asm volatile("iret");
}

void syscallWrite(struct StackFrame *sf) {
    switch (sf->ecx) {  // file descriptor
        case 0:
            syscallPrint(sf);
            break;  // for STD_OUT
        default:
            break;
    }
}

// Attention:
// This is optional homework, because now our kernel can not deal with
// consistency problem in syscallPrint. If you want to handle it, complete this
// function. But if you're not interested in it, don't change anything about it
void syscallPrint(struct StackFrame *sf) {
    int sel = sf->ds;  // TODO segment selector for user data, need further
                       // modification
    char *str = (char *)sf->edx;
    int size = sf->ebx;
    int i = 0;
    int pos = 0;
    char character = 0;
    uint16_t data = 0;
    asm volatile("movw %0, %%es" ::"m"(sel));
    for (i = 0; i < size; i++) {
        asm volatile("movb %%es:(%1), %0" : "=r"(character) : "r"(str + i));
        if (character == '\n') {
            displayRow++;
            displayCol = 0;
            if (displayRow == 25) {
                displayRow = 24;
                displayCol = 0;
                scrollScreen();
            }
        } else {
            data = character | (0x0c << 8);
            pos = (80 * displayRow + displayCol) * 2;
            asm volatile("movw %0, (%1)" ::"r"(data), "r"(pos + 0xb8000));
            displayCol++;
            if (displayCol == 80) {
                displayRow++;
                displayCol = 0;
                if (displayRow == 25) {
                    displayRow = 24;
                    displayCol = 0;
                    scrollScreen();
                }
            }
        }
        // asm volatile("int $0x20"); //XXX Testing irqTimer during syscall
        // asm volatile("int $0x20":::"memory"); //XXX Testing irqTimer during
        // syscall
    }

    updateCursor(displayRow, displayCol);
    // TODO take care of return value
    return;
}

void syscallFork(struct StackFrame *sf) {
    // TODO in lab3
    // try to find usable pcb
    int usable_pcb = -1;
    for(int i = 1; i < MAX_PCB_NUM; i++) {
        if(pcb[i].state == STATE_DEAD) {
            usable_pcb = i;
            break;
        }
    }
    if(usable_pcb == -1) {
        pcb[current].regs.eax = -1; // no usable pcb
        return;
    }
    // putChar('F');
    // putChar('0'+usable_pcb);
    // copy code and data segment
    for (int j = 0; j < 0x100000; j++) {
         *(uint8_t *)(j + (usable_pcb + 1) * 0x100000) = *(uint8_t *)(j + (current +1) * 0x100000);
    }
    // set pcb
    pcb[current].regs.eax = usable_pcb; // parent proc
    // pcb[usable_pcb].regs = pcb[current].regs;
    pcb[usable_pcb] = pcb[current];
    // pcb[usable_pcb].regs = pcb[current].regs;
    pcb[usable_pcb].regs.eflags = pcb[current].regs.eflags;
    pcb[usable_pcb].regs.edx = pcb[current].regs.edx;
    pcb[usable_pcb].regs.ecx = pcb[current].regs.ecx;
    pcb[usable_pcb].regs.ebx = pcb[current].regs.ebx;
    pcb[usable_pcb].regs.esp = pcb[current].regs.esp;
    pcb[usable_pcb].regs.ebp = pcb[current].regs.ebp;
    pcb[usable_pcb].regs.edi = pcb[current].regs.edi;
    pcb[usable_pcb].regs.esi = pcb[current].regs.esi;
    pcb[usable_pcb].regs.eip = pcb[current].regs.eip;
    pcb[usable_pcb].regs.cs = USEL(usable_pcb*2+1);
    pcb[usable_pcb].regs.ds = USEL(usable_pcb*2+2);
    pcb[usable_pcb].regs.es = USEL(usable_pcb*2+2);
    pcb[usable_pcb].regs.gs = USEL(usable_pcb*2+2);
    pcb[usable_pcb].regs.fs = USEL(usable_pcb*2+2);
    pcb[usable_pcb].regs.ss = USEL(usable_pcb*2+2); // !!!

    // if(pcb[usable_pcb].regs.eip == pcb[current].regs.eip) {
    //     putChar('P');
    // }

    pcb[usable_pcb].regs.eax = 0; // child proc

    pcb[usable_pcb].pid = usable_pcb;
    pcb[usable_pcb].sleepTime = 0;
    pcb[usable_pcb].timeCount = 0;
    pcb[usable_pcb].prevStackTop = (uint32_t)&(pcb[usable_pcb].stackTop);
    pcb[usable_pcb].stackTop = (uint32_t)&(pcb[usable_pcb].regs);
    pcb[usable_pcb].state = STATE_RUNNABLE;
}

void syscallSleep(struct StackFrame *sf) {
    // TODO in lab3
    // putChar('S');
    // putChar('0'+current);
    uint32_t time = sf->ecx;
    pcb[current].sleepTime = time;
    pcb[current].timeCount = 0;
    pcb[current].state = STATE_BLOCKED;
    asm volatile("int $0x20");
}

void syscallExit(struct StackFrame *sf) {
    // TODO in lab3
    pcb[current].state = STATE_DEAD;
    asm volatile("int $0x20");
}
