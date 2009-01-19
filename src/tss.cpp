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
#include "nmi.h"

Tss Tss::run;
Tss Tss::dbf;

#ifdef NIXON
Tss Tss::nmi;
Tss Tss::dbg;
#endif

void Tss::build()
{
    extern char task_gate_handler;
#ifdef NIXON
    extern char nmi_handler;
    extern char dbg_handler;
#endif

    dbf.cr3     = Buddy::ptr_to_phys (Ptab::current());
    dbf.eip     = reinterpret_cast<uint32>(&task_gate_handler);
    // If we want to do anything else instead of panic'ing using the
    // kernel stack might be A Bad Idea(tm).
    dbf.esp     = KSTCK_EADDR;
    dbf.eflags  = 2;
    dbf.cs      = SEL_KERN_CODE;
    dbf.ds      = SEL_KERN_DATA;
    dbf.es      = SEL_KERN_DATA;
    dbf.ss      = SEL_KERN_DATA;

#ifdef NIXON
    // Contruct TSS for NMI handler.
    nmi.cr3     = Buddy::ptr_to_phys (Ptab::current());
    nmi.eip     = reinterpret_cast<uint32>(&nmi_handler);
    nmi.esp     = NMISTCK_EADDR;
    // The spec says bit 1 is reserved and should be set to 1.
    nmi.eflags  = 2;
    nmi.cs      = SEL_KERN_CODE;
    nmi.ds      = SEL_KERN_DATA;
    nmi.es      = SEL_KERN_DATA;
    nmi.ss      = SEL_KERN_DATA;

    // Construct TSS for debug handler. The debug handler just blocks
    // in HLT and needs no working environment.
    dbg.cr3     = Buddy::ptr_to_phys (Ptab::current());
    dbg.eip     = reinterpret_cast<uint32>(&dbg_handler);
    dbg.eflags  = 2;
    dbg.cs      = SEL_KERN_CODE;
    dbg.ds      = SEL_KERN_DATA;
    dbg.es      = SEL_KERN_DATA;
    dbg.ss      = SEL_KERN_DATA;
#endif  // NIXON

    // Construct the "normal operation" TSS used by anything except
    // interrupt handling.
    run.ss0     = SEL_KERN_DATA;
    run.esp0    = KSTCK_EADDR;
    run.iobm    = static_cast<uint16>(IOBMP_SADDR - reinterpret_cast<mword>(&run));
}
