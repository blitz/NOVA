/*
 * Object Space
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
#include "space_obj.h"
#include "state.h"

bool Space_obj::insert (unsigned idx, Capability cap)
{
    mword virt = idx_to_virt (idx);

    Paddr phys;
    if (!space_mem->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0)) {
        phys = Buddy::ptr_to_phys (Buddy::allocator.alloc (0, Buddy::FILL_0));
        space_mem->insert (virt & ~PAGE_MASK,
                           phys,
                           PAGE_SIZE,
                           Ptab::Attribute (Ptab::ATTR_NOEXEC |
                                            Ptab::ATTR_WRITABLE));

        phys |= virt & PAGE_MASK;
    }

    return Atomic::cmp_swap_mp (static_cast<Capability *>(Buddy::phys_to_ptr (phys)), Capability(), cap);
}

bool Space_obj::remove (unsigned idx, Capability cap)
{
    mword virt = idx_to_virt (idx);

    Paddr phys;
    if (!space_mem->lookup (virt, phys) || (phys & ~PAGE_MASK) == reinterpret_cast<Paddr>(&FRAME_0))
        return false;

    return Atomic::cmp_swap_mp (static_cast<Capability *>(Buddy::phys_to_ptr (phys)), cap, Capability());
}

void Space_obj::page_fault (Exc_regs *regs)
{
    assert (!(regs->err & Paging::ERROR_WRITE));

    // XXX: Check sync with PD master page table before allocating anything

    Pd::current->Space_mem::insert (regs->cr2 & ~PAGE_MASK,
                                    reinterpret_cast<Paddr>(&FRAME_0),
                                    PAGE_SIZE,
                                    Ptab::ATTR_NOEXEC);
}
