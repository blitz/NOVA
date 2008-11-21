/*
 * Memory Space
 *
 * Copyright (C) 2007-2008, Udo Steinberg <udo@hypervisor.org>
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

#include "memory.h"
#include "pd.h"
#include "space_mem.h"
#include "state.h"

Mr Space_mem::array[0x100000];

Space_mem::Space_mem()
{
    // Create ptab for this CPU
    master = percpu[Cpu::id] = new (true) Ptab;

    // Sync cpu-local range
    master->sync_local();

    // Sync kernel code and data
    master->sync_master_range (LINK_ADDR, LOCAL_SADDR);
}

void Space_mem::page_fault (Exc_regs *regs)
{
    mword addr = regs->cr2;

    trace (TRACE_MEMORY, "#PF (MEM) PD:%p ADDR:%#lx", Pd::current, addr);

    Paddr phys;
    if (Ptab::master()->lookup (addr, phys))
        Ptab::current()->sync_master (addr);
}
