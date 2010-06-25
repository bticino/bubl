#include <bubl/io.h>
#include <bubl/hw.h>
#include <bubl/hw-misc.h>

void bubl_main(void)
{

	misc_setup0();
	pll1_setup();
	pll2_setup();
	/* FIXME: set up RAM */
	misc_setup1();
	serial_setup();

	puts("Here I am\n");
	while (1)
		;
}
