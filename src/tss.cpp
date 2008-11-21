/*
 * Task State Segment (TSS)
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

#include "memory.h"
#include "ptab.h"
#include "tss.h"

Tss Tss::run;
Tss Tss::dbf;

void Tss::build()
{
    extern char task_gate_handler;

    dbf.cr3     = Buddy::ptr_to_phys (Ptab::current());
    dbf.eip     = reinterpret_cast<uint32>(&task_gate_handler);
    dbf.esp     = KSTCK_EADDR;
    dbf.eflags  = 2;
    dbf.cs      = SEL_KERN_CODE;
    dbf.ds      = SEL_KERN_DATA;
    dbf.es      = SEL_KERN_DATA;
    dbf.ss      = SEL_KERN_DATA;

    run.ss0     = SEL_KERN_DATA;
    run.esp0    = KSTCK_EADDR;
    run.iobm    = static_cast<uint16>(IOBMP_SADDR - reinterpret_cast<mword>(&run));
}
