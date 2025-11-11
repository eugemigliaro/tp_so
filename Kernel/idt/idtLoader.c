// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <idtLoader.h>

#pragma pack(push)
#pragma pack(1)

typedef struct {
   uint16_t offset_l;
   uint16_t selector;
   uint8_t  zero;
   uint8_t  access;
   uint16_t offset_m;
   uint32_t offset_h;
   uint32_t other_zero;
} DESCR_INT;

#pragma pack(pop)

DESCR_INT * idt = (DESCR_INT *) 0;

static void setup_IDT_entry(int index, uint64_t offset);

void load_idt() {
	_cli();
	setup_IDT_entry(0x00, (uint64_t)&_exceptionHandler00);
	setup_IDT_entry(0x06, (uint64_t)&_exceptionHandler06);

	setup_IDT_entry(0x20, (uint64_t) &_irq00Handler); 
	setup_IDT_entry(0x21, (uint64_t) &_irq01Handler);
	setup_IDT_entry(0x80, (uint64_t) &_irq80Handler);

	picMasterMask(KEYBOARD_PIC_MASTER & TIMER_PIC_MASTER);
	picSlaveMask(NO_INTERRUPTS);
			
	_sti();
}

static void setup_IDT_entry(int index, uint64_t offset) {
	idt[index].offset_l = offset & 0xFFFF;
	idt[index].selector = 0x08;
	idt[index].zero = 0;
	idt[index].access = ACS_INT;
	idt[index].offset_m = (offset >> 16) & 0xFFFF;
	idt[index].offset_h = (offset >> 32) & 0xFFFFFFFF;
	idt[index].other_zero = 0;
}
