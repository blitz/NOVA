/*
 * Virtual Translation Lookaside Buffer (VTLB)
 *
 * Copyright (C) 2008, Udo Steinberg <udo@hypervisor.org>
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
#include "cpu.h"
#include "pd.h"
#include "state.h"
#include "util.h"
#include "vtlb.h"

size_t Vtlb::phys_to_host (mword phys, Paddr &host)
{
    if (EXPECT_FALSE (phys >= LINK_ADDR))
        return 0;

    return Pd::current->Space_mem::lookup (phys, host);
}

size_t Vtlb::virt_to_phys (Exc_regs *regs, mword virt, mword error, mword &phys, mword &attr)
{
    if (EXPECT_FALSE (!(regs->cr0_shadow & Cpu::CR0_PG))) {
        phys = virt;
        return ~0ul;
    }

    bool pse = regs->cr4_shadow & Cpu::CR4_PSE;
    bool pge = regs->cr4_shadow & Cpu::CR4_PGE;

    mword bits = (error & (ERROR_USER | ERROR_WRITE)) | ERROR_PRESENT;

    unsigned lev = levels;

    for (Vtlb *pte = reinterpret_cast<Vtlb *>(regs->cr3_shadow);; pte = reinterpret_cast<Vtlb *>(pte->addr())) {

        unsigned shift = --lev * bits_per_level + 12;
        pte += virt >> shift & ((1ul << bits_per_level) - 1);
        attr &= pte->attr();

        if (EXPECT_FALSE ((attr & bits) != bits))
            return 0;

        if (EXPECT_FALSE (!pte->accessed()))
            Atomic::set_mask_mp (pte->val, ATTR_ACCESSED);

        if (EXPECT_FALSE (lev && (!pse || !pte->superpage())))
            continue;

        if (EXPECT_FALSE (!pte->dirty())) {
            if (EXPECT_FALSE (error & ERROR_WRITE))
                Atomic::set_mask_mp (pte->val, ATTR_DIRTY);
            else
                attr &= ~ATTR_WRITABLE;
        }

        attr |= pte->attr() & ATTR_UNCACHEABLE;

        if (EXPECT_TRUE (pge))
            attr |= pte->attr() & ATTR_GLOBAL;

        size_t size = 1ul << shift;

        phys = pte->addr() | (virt & (size - 1));

        return size;
    }
}

Vtlb::Reason Vtlb::miss (Exc_regs *regs, mword &virt, mword error)
{
    mword attr = ATTR_USER | ATTR_WRITABLE | ATTR_PRESENT;
    mword phys;
    Paddr host;

    size_t gsize = virt_to_phys (regs, virt, error, phys, attr);
    if (EXPECT_FALSE (!gsize))
        return GLA_GPA;

    size_t hsize = phys_to_host (phys, host);
    if (EXPECT_FALSE (!hsize)) {
        virt = phys;
        return GPA_HPA;
    }

    size_t size = min (gsize, hsize);

    unsigned lev = levels;

    for (Vtlb *tlb = regs->vtlb;; tlb = static_cast<Vtlb *>(Buddy::phys_to_ptr (tlb->addr()))) {

        unsigned shift = --lev * bits_per_level + 12;
        tlb += virt >> shift & ((1ul << bits_per_level) - 1);

        if (lev && size != 1ul << shift) {

            if (tlb->superpage())
                tlb->val = Buddy::ptr_to_phys (new Vtlb) | ATTR_PTAB;

            else if (!tlb->present()) {
                static_cast<Vtlb *>(Buddy::phys_to_ptr (tlb->addr()))->flush (tlb->global());
                tlb->val = tlb->addr() | ATTR_PTAB;
            }

            continue;
        }

        if (lev) {

            if (!tlb->superpage())          // XXX: delete whole subtree
                delete static_cast<Vtlb *>(Buddy::phys_to_ptr (tlb->addr()));

            attr |= ATTR_SUPERPAGE;
        }

        tlb->val = (host & ~(size - 1)) | attr | ATTR_LEAF;

        return SUCCESS;
    }
}
