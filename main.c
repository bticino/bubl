#include <bubl/io.h>
#include <bubl/hw.h>
#include <bubl/hw-misc.h>
#include <bubl/tools.h>

void bubl_main(void)
{

	misc_setup0();
	pll1_setup();
	pll2_setup();
	/* FIXME: set up RAM */
	misc_setup1();
	serial_setup();

	printk("Function %s (%s:%i), compile date %s\n",
	       __FILE__, __func__, __LINE__, __DATE__);

	/* Nothing more to do by now */
	while (1)
		;
}
