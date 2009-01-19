/*
 * Global Descriptor Table (GDT)
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

#include "gdt.h"
#include "memory.h"
#include "selectors.h"
#include "tss.h"

Gdt Gdt::gdt[SEL_MAX >> 3] ALIGNED(8);

void Gdt::build()
{
    gdt[SEL_KERN_CODE >> 3].set (CODE_XRA, PAGES, BIT_32, 0, 0, ~0ul);
    gdt[SEL_KERN_DATA >> 3].set (DATA_RWA, PAGES, BIT_32, 0, 0, ~0ul);

    gdt[SEL_USER_CODE >> 3].set (CODE_XRA, PAGES, BIT_32, 3, 0, ~0ul);
    gdt[SEL_USER_DATA >> 3].set (DATA_RWA, PAGES, BIT_32, 3, 0, ~0ul);

    gdt[SEL_TSS_RUN >> 3].set (SYS_TSS, BYTES, BIT_16, 0, reinterpret_cast<mword>(&Tss::run), IOBMP_EADDR - reinterpret_cast<mword>(&Tss::run));
    gdt[SEL_TSS_DBF >> 3].set (SYS_TSS, BYTES, BIT_16, 0, reinterpret_cast<mword>(&Tss::dbf), sizeof (Tss) - 1);
#ifdef NIXON
    gdt[SEL_TSS_NMI >> 3].set (SYS_TSS, BYTES, BIT_16, 0, reinterpret_cast<mword>(&Tss::nmi), sizeof (Tss) - 1);
    gdt[SEL_TSS_DBG >> 3].set (SYS_TSS, BYTES, BIT_16, 0, reinterpret_cast<mword>(&Tss::dbg), sizeof (Tss) - 1);
#endif
}
