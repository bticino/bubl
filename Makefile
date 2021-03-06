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

CFLAGS = -Os -Wall -ggdb -ffreestanding -Iinclude -DCOMPLETELY_QUIET

all: $(ALL)

# objects that make the program
obj-s := boot.o reset.o
obj-c += header.o pll.o hw-misc.o serial.o ddr.o timer.o adc.o board.o main.o
obj-c += s_record.o
obj-c += lib/ctype.o lib/string.o
obj-c += lib/sprintf.o lib/strtol.o lib/printk.o lib/vsprintf.o lib/div64.o
obj-c += drivers/nand/nand.o drivers/nand/nand_base.o drivers/nand/nand_ids.o drivers/nand/davinci_nand.o
obj-c += drivers/mmc/mmc.o drivers/mmc/davinci_mmc.o drivers/mmc/mmc-u-boot-glue.o

# we have some division
obj-s += libgcc/_udivsi3.o libgcc/_divsi3.o libgcc/_ashldi3.o libgcc/_lshrdi3.o
obj-c += libgcc/div0.o

obj-y := $(obj-s) $(obj-c)

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
	rm -f $(ALL) $(obj-y) *~ */*~ include/*/*~ $(MAP) .depend

-include .depend

.depend: $(obj-s:.o=.S) $(obj-c:.o=.c)
	$(CC) $(CFLAGS) -M $^ > $@
