##########################################################################
#                        ELEC4710 - REALTIME SYSTEMS
#
# THIS FILE WAS CREATED AND IS FOR ELEC4710 - REALTIME SYSTEMS
# PROJECT ASSIGNMENT
#
# This file is the Makefile used to build the Operating System
#
# Author: Colin Goudie
#
##########################################################################
PROG = kernel.bin
SRCS = kernel/kernel.c kernel/idt.c kernel/pic.c kernel/timer.c \
       kernel/idle.c kernel/sys.c \
       kernel/taskm/sched.c kernel/taskm/sema.c \
       kernel/mm/mm.c \
	lib/stdio/vsprintf.c \
	lib/string/isdigit.c lib/string/strlen.c lib/string/strnlen.c \
   lib/string/memset.c lib/string/strcpy.c lib/string/memcpy.c \
   app/user.c \
   os/os.c
OBJS = $(SRCS:.c=.o)
INCS = -I./include -I./ -I./include/lib
AS = gcc
ASFLAGS = -nostdinc
CC = cc
CFLAGS = -fwritable-strings -nostdinc -Wall -O2
LD = ld
LINKFILE = link.ld
LDFLAGS = -nostdlib -nodefaultlibs -N -T link.ld -Map map

all: boot init context systemcall $(OBJS)	build

%.o: %.c
				$(CC) -c $(CFLAGS) $(INCS) $*.c -o $*.o

build: $(OBJS)
				$(LD)  $(LDFLAGS) -o $(PROG) boot.o init.o context_switch.o systemcall.o $(OBJS)

boot:	boot.S
				$(AS) -c $(INCS) $(ASFLAGS) $? -o boot.o

init:	init.S
				$(AS) -c $(INCS) $(ASFLAGS) $? -o init.o

systemcall:	kernel/system_call.S
				$(AS) -c $(INCS) $(ASFLAGS) $? -o systemcall.o

context:	kernel/taskm/context_switch.S
				$(AS) -c $(INCS) $(ASFLAGS) $? -o context_switch.o

clean:
				rm -f *.o $(OBJS) $(PROG)

