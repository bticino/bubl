# file names
NAME	= bubl
ELF	= $(NAME)
LDS	= $(NAME).lds
MAP	= $(NAME).map
BIN	= $(NAME).bin
SREC	= $(NAME).srec

ALL	= $(ELF) $(BIN)

# tool names
HOSTCC		= gcc
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

CFLAGS = -O2 -Wall -ggdb -ffreestanding -Iinclude


all: $(ALL)

# objects that make the program
obj-y = header.o boot.o pll.o hw-misc.o serial.o main.o

# main rule
$(ELF): $(obj-y) $(LDS) 
	$(LD) -Map $(MAP) -T $(LDS) $(obj-y) $(LDFLAGS) -o $@

# binary targets
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $^ $@

$(SREC): $(ELF)
	$(OBJCOPY) -O srec $^ $@

# other rules
clean:
	rm -f $(ALL) *.o *~ */*~ include/*/*~ $(MAP)
