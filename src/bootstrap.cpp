/*
 * Bootstrap Code
 *
 * Copyright (C) 2005-2008, Udo Steinberg <udo@hypervisor.org>
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

#include "acpi.h"
#include "compiler.h"
#include "cpu.h"
#include "ec.h"
#include "extern.h"
#include "hip.h"
#include "pd.h"
#include "sc.h"

extern "C" NORETURN
void bootstrap()
{
    Cpu::init();

    // XXX: Rendezvous all CPUs and sync APIC timers
    Acpi::delay (500);

    // Create root task
    if (Cpu::bsp) {
        Hip::add_checksum();
        Pd::root = new Pd;
        Ec *root_ec = new Ec (Pd::root, Ec::root_invoke, LINK_ADDR - 2 * PAGE_SIZE);
        Sc *root_sc = new Sc (root_ec, Sc::default_prio, Sc::default_quantum);
        root_sc->ready_enqueue();
    }

    // Create idle EC
    Ec::current = new Ec (&Pd::kern, Ec::idle);
    Sc::current = new Sc (Ec::current, 0, Sc::default_quantum);

    Sc::schedule();
}
