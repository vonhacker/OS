/*************************************************************************
 *                            -=-=-= kernel.c =-=-=-
 *
 * THIS FILE WAS CREATED FOR ELEC4710 - REALTIME SYSTEMS
 * MAJOR PROJECT ASSIGNMENT
 *
 * This file contains the kernel main function that is used to initialise
 * the realtime kernel/Operating System 
 *
 * Compiler: gcc
 *
 * Author: Colin Goudie
 **************************************************************************/
#include <os/multiboot.h>
#include <os/kernel.h>
#include <os/platform.h>
#include <os/mm/paging.h>
#include <os/mm/mm.h>
#include <os/idt.h>
#include <os/pic.h>
#include <os/timer.h>
#include <os/taskm/sched.h>
#include <console.h>
#include <stdarg.h>
#include <stdio.h>

extern void test_function();

/* Number of System Ticks */
static ulong system_ticks = 0;
/* The cached state of the interrupts. This starts of as FALSE
   as that is the state of the interrupts when we enter our kernel */
static uchar interrupts_enabled = FALSE;
/** This idea is taken from the linux 0.01 kernel. We set up a 
user stack but we also use it as the starting kernel stack too */
long    user_stack [ PAGE_SIZE >> 2 ];

long    test_stack [PAGE_SIZE >> 2 ];
/** Initialise a Stack Descriptor pointing at the top of the user_stack
(PAGE>>2)  and pointing to our data segment (0x10) */
struct stack start_stack = { &user_stack[PAGE_SIZE >> 2], KERNEL_DATA };
/**
 * This will be the global tss structure
 */
struct tss global_tss;
/**
 * This is our gdt (created in init.S)
 */
extern desc_table gdt;

typedef struct test_structure_1
{
    uint one;
    uint two;
} test_struc1;

typedef struct test_structure_2
{
    uint one;
    uint two;
    uint three;
} test_struc2;
extern void idle_task(void* ptr);
int k_main(multiboot_info_t* info) // like main in a normal C program
{
    struct process* idle;
	k_clear_screen();
    k_printf("booting...", 0);

    klprintf(2, "Low Memory: %d", info->mem_lower);
    klprintf(3, "High Memory: %d", info->mem_upper);

    init_mm();
    init_idt();
    init_sched();

    //Screwing around with jumping to user mode
#define move_to_user_mode(ss, stk, cs, eip)\
    asm("movl %0, %%eax\n" \
    "mov %%ax, %%ds\n" \
    "mov %%ax, %%es\n" \
    "mov %%ax, %%fs\n" \
    "mov %%ax, %%gs\n" \
    "pushl %0\n" \
    "pushl %1\n"\
    "pushl %2\n"\
    "pushl %3\n"\
    "pushl %4\n"\
    "iret\n"\
    ::"m"(ss), "m"(stk), "i"(2+(1<<9)), "m"(cs), "m"(eip):"%eax")

    reprogram_pic( 0x20, 0x28 );

    init_timer();

    enable_irq( 0 );

    klprintf(19, "Starting Kernel");

    enable();

    idle = get_idle_task();

    //move_to_user_mode( &test_stack [PAGE_SIZE >> 2 ], &test_function);
    //move_to_user_mode( get_idle_task()->thread_list->task_state.esp, get_idle_task()->thread_list->task_state.eip);
    move_to_user_mode(idle->thread_list->task_state.ss, 
                      idle->thread_list->task_state.esp, 
                      idle->thread_list->task_state.cs, 
                      idle->thread_list->task_state.eip);
    //Enter temp idle loop
    for (;;)
        asm("hlt");

    return 0;    
};

void k_clear_screen() // clear the entire text screen
{
	char *vidmem = (char *) VIDEO_MEMORY;
	unsigned int i=0;
	while(i < (CONSOLE_WIDTH * CONSOLE_HEIGHT * CONSOLE_DEPTH))
	{
		vidmem[i]=' ';
		i++;
		vidmem[i]=WHITE;
		i++;
	};
};

void klprintf(uint line, uchar* fmt, ...)
{
    //Set aside a large buffer for input
    uchar buffer[1024];

    va_list arguments;
    va_start(arguments, fmt);

    vsprintf( buffer, fmt, arguments );

    k_printf(buffer, line);

    va_end(arguments);
}

unsigned int k_printf(char *message, unsigned int line) // the message and then the line #
{
	char *vidmem = (char *) VIDEO_MEMORY;
	unsigned int i=0;

	i=(line * CONSOLE_WIDTH * CONSOLE_DEPTH);

	while(*message!=0)
	{
		if(*message=='\n') // check for a new line
		{
			line++;
			i=(line * CONSOLE_WIDTH * CONSOLE_DEPTH);
			*message++;
		} else {
			vidmem[i]=*message;
			*message++;
			i++;
			vidmem[i]=WHITE;
			i++;
		}
	}

	return(1);
}

ulong get_system_ticks()
{
    return system_ticks;
}

void inc_system_ticks()
{
    system_ticks++;
}

void disable()
{
    asm volatile ("cli");
    interrupts_enabled = FALSE;
}

void enable()
{
    asm volatile ("sti");
    interrupts_enabled = TRUE;
}

uchar return_interrupt_status()
{
    return interrupts_enabled;
}

ulong save_flags()
{
    ulong result;
    asm volatile (  "pushfl\n\t"
                    "popl %0"
                    :"=r" (result) :: "memory");
    return result;
}

void restore_flags(ulong flags)
{
    asm volatile (  "pushl  %0\n\t"
                    "popfl"
                    :: "r" (flags) : "memory");
}

void create_gdt_segment_descriptor(   uint segment_index,
                                    uint base_address,
                                    uint segment_limit,
                                    uint segment_type,
                                    uint privilege_level,
                                    uint present,
                                    uint granularity )
{
    gdt[segment_index].descripts.seg_descriptor.segment_limit_15_00 = segment_limit & 0x0000FFFF;
    gdt[segment_index].descripts.seg_descriptor.base_address_15_00 = base_address & 0x0000FFFF;
    gdt[segment_index].descripts.seg_descriptor.base_address_23_16 = (base_address & 0x00FF0000) >> 16;
    gdt[segment_index].descripts.seg_descriptor.segment_type = segment_type;
    gdt[segment_index].descripts.seg_descriptor.descriptor_type = 1; //Code OR Data
    gdt[segment_index].descripts.seg_descriptor.dpl = privilege_level;
    gdt[segment_index].descripts.seg_descriptor.present = present;
    gdt[segment_index].descripts.seg_descriptor.segment_limit_19_16 = (segment_limit & 0x00FF0000) >> 16;
    gdt[segment_index].descripts.seg_descriptor.avl = 1;
    gdt[segment_index].descripts.seg_descriptor.default_operation = 1;  //32 bit segment
    gdt[segment_index].descripts.seg_descriptor.zero = 0;
    gdt[segment_index].descripts.seg_descriptor.granularity = granularity;
    gdt[segment_index].descripts.seg_descriptor.base_address_31_24 = (base_address & 0xFF000000) >> 24;
}


