/*
 * Initialization Code
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
#include "buddy.h"
#include "cmdline.h"
#include "compiler.h"
#include "extern.h"
#include "gsi.h"
#include "hip.h"
#include "idt.h"
#include "keyb.h"
#include "memory.h"
#include "pic.h"
#include "ptab.h"
#include "ptab_boot.h"
#include "stdio.h"
#include "types.h"

char const *version = "NOVA 0.0.1";

extern "C" INIT
mword kern_ptab_setup()
{
    Ptab *ptab = new (true) Ptab;

    ptab->map (CPULC_SADDR,
               Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)),
               CPULC_EADDR - CPULC_SADDR,
               Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                Ptab::ATTR_GLOBAL |
                                Ptab::ATTR_WRITABLE));

    ptab->map (KSTCK_SADDR, 
               Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0)),
               KSTCK_EADDR - KSTCK_SADDR,
               Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                Ptab::ATTR_GLOBAL |
                                Ptab::ATTR_WRITABLE));

    ptab->sync_master_range (LINK_ADDR, LOCAL_SADDR);

    return Buddy::ptr_to_phys (ptab);
}

extern "C" INIT REGPARM (2)
void init_ilp (mword mbi, unsigned secure)
{
    Ptab_boot::init();

    Paging::enable();

    Cpu::secure = secure;

    extern void (*_ctors_end)(), (*_ctors_start)();
    for (void (**func)() = &_ctors_end; func != &_ctors_start; (*--func)()) ;

    memset (reinterpret_cast<void *>(&PAGE_0),  0,  PAGE_SIZE);
    memset (reinterpret_cast<void *>(&PAGE_1), ~0u, PAGE_SIZE);

    serial.init();
    screen.init();

    printf ("\f%s: %s %s [%s]\n\n", version, __DATE__, __TIME__, COMPILER);

    Hip::build (mbi);

    Pic::master.init (VEC_GSI);
    Pic::slave.init  (VEC_GSI + 8);

    Idt::build();
    Gsi::setup();
    Acpi::setup();

    Keyb::init();
}

extern "C" INIT
void init_rlp()
{
    Paging::enable();
}
