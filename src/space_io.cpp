/*
 * I/O Space
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

#include "atomic.h"
#include "extern.h"
#include "pd.h"
#include "space_io.h"
#include "state.h"

Mr Space_io::array[0x10000];

bool Space_io::insert (unsigned idx)
{
    mword virt = idx_to_virt (idx);

    Paddr phys;
    if (!space_mem->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_1)) {
        phys = Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_1));
        space_mem->insert (virt & ~PAGE_MASK,
                           phys,
                           PAGE_SIZE,
                           Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                            Ptab::ATTR_WRITABLE));

        phys |= virt & PAGE_MASK;
    }

    // Clear the bit, permitting access
    return Atomic::test_clr_bit_mp (*static_cast<mword *>(Buddy::phys_to_ptr (phys)), idx_to_bit (idx));
}

bool Space_io::remove (unsigned idx)
{
    mword virt = idx_to_virt (idx);

    Paddr phys;
    if (!space_mem->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_1))
        return false;

    // Set the bit, prohibiting access
    return !Atomic::test_set_bit_mp (*static_cast<mword *>(Buddy::phys_to_ptr (phys)), idx_to_bit (idx));
}

void Space_io::page_fault (Exc_regs *regs)
{
    // Should never get a write fault during lookup
    assert (!(regs->err & Paging::ERROR_WRITE));

    // XXX: Check sync with PD master page table before allocating anything

    // Read fault on non-present page; map 1-page r/o
    Pd::current->Space_mem::insert (regs->cr2 & ~PAGE_MASK,
                                    reinterpret_cast<Paddr>(&FRAME_1),
                                    PAGE_SIZE,
                                    Ptab::ATTR_NOEXEC);
}
