/*
 * Copyright (C) 2009, Julian Stecklina
 *
 *   ((
 *    ))     This file is COFFEEWARE. As long as you retain this notice
 *  |   |o)  you can do whatever you want with this code. If you think,
 *  |___|jgs it's worth it, you may buy the author a coffee in return.
 *
 */

#ifdef NIXON

        /* AT&T syntax is for compilers, so use GAS' crappy emulation
	of MASM/TASM. */
        .intel_syntax noprefix

        .text
        .globl dbg_handler

dbg_handler:
        /* nmi_command_space.inside_dbg */
        mov [dword ptr nmi_command_space + 6*4], 1

        /* We can use hlt even though IF is disabled, since we have to
	   be woken up by a #NMI anyway which forces us out of this
           loop. */
1:      hlt
        jmp 1b

#endif /* NIXON */

/* EOF */
