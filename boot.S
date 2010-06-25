
.section ".text.boot"	
	
	ldr	sp, = _stack_end

clearbss:
	ldr	r1, =__bss_start - 4
	ldr	r2, =__bss_end
	mov	r0, #0
1:	str	r0, [r1, #4]!
	cmp	r1, r2
	blt	1b
	bl	bubl_main
	/* Bubl_main should not return... */
	adr	r0, msg
	bl	puts
0:
	b 0b

msg:	.asciz "Panic: main returned\n"
