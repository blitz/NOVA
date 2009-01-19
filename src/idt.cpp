/*
 * Interrupt Descriptor Table (IDT)
 *
 * Copyright (C) 2006-2008, Udo Steinberg <udo@hypervisor.org>
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#include "extern.h"
#include "idt.h"
#include "selectors.h"

Idt Idt::idt[VEC_MAX] ALIGNED(8);

void Idt::build()
{
    mword *ptr = handlers;

    for (unsigned vector = 0; vector < VEC_MAX; vector++, ptr++) {
        switch (vector) {
#ifdef NIXON
        case 1:                // #DB Debug exception
        case 3:                // #BP Breakpoint Exception
            idt[vector].set (SYS_TASK_GATE, 0, SEL_TSS_DBG, 0);
            break;
        case 2:                // #NMI Non-maskable Interrupt
            idt[vector].set (SYS_TASK_GATE, 0, SEL_TSS_NMI, 0);
            break;
#endif
        default:
            if (*ptr)
                idt[vector].set (SYS_INTR_GATE, *ptr & 3, SEL_KERN_CODE, *ptr & ~3);
            else
                idt[vector].set (SYS_TASK_GATE, 0, SEL_TSS_DBF, 0);
        }
    }
}
