/*
 * IA32 Page Table
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

#include "assert.h"
#include "buddy.h"
#include "memory.h"
#include "pd.h"
#include "ptab.h"
#include "stdio.h"
#include "tlb.h"

Slab_cache Ptab::cache ("Ptab", 32, 32);
Paddr Ptab::remap_addr = static_cast<Paddr>(~0ull);

/*
 * In order to track when a PTAB is no longer in use, we must know how many
 * entries are currently populated in each PT. Tracking must occur on a
 * per-frame basis because a PTAB could potentially be shared by multiple
 * PDIRs. The entry counter could reside in the per-frame buddy meta data.
 */
bool Ptab::map (mword virt, Paddr phys, size_t size, Attribute attrib)
{
    while (size) {

        trace (TRACE_PTAB, "(M) %#010lx LIN:%#010lx PHY:%#010llx A:%#05llx S:%#x",
               Buddy::ptr_to_phys (this),
               virt,
               static_cast<uint64>(phys),
               static_cast<uint64>(attrib),
               size);

        unsigned lev = levels;

        for (Ptab *pte = this;; pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr()))) {

            unsigned shift = --lev * bits_per_level + 12;
            pte += virt >> shift & ((1ul << bits_per_level) - 1);
            size_t mask = (1ul << shift) - 1;

            if (size > mask && !((phys | virt) & mask)) {

                Paddr a = attrib | ATTR_LEAF;

                if (lev)
                    a |= ATTR_SUPERPAGE;

                bool flush_tlb = false;

                if (pte->present()) {

                    // XXX: Could be an EMPTY pagetable
                    if (lev && !pte->superpage())
                        panic ("Overmap PT with SP\n");

                    // Physical address change or attribute downgrade
                    if (pte->addr() != phys || (pte->val ^ ATTR_INVERTED) & (pte->val ^ a) & ATTR_ALL)
                        flush_tlb = true;
                }

                // XXX: use cmpxchg
                pte->val = phys | (a & ptab_bits[lev]);

                if (flush_tlb)
                    Tlb::flush (virt);

                trace (TRACE_PTAB, "   -> L:%u S:%u PTE:%#010lx PHY:%#010llx A:%#05llx FRAME",
                       lev, shift,
                       Buddy::ptr_to_phys (pte),
                       static_cast<uint64>(pte->addr()),
                       static_cast<uint64>(pte->attr()));

                mask++;
                size -= mask;
                virt += mask;
                phys += mask;
                break;
            }

            if (!lev)
                panic ("Unsupported SIZE\n");

            // XXX: use cmpxchg
            if (!pte->present())
                pte->val = Buddy::ptr_to_phys (new Ptab) | (ATTR_PTAB & ptab_bits[lev]);

            else if (pte->superpage())
                panic ("Overmap SP with PT\n");

            trace (TRACE_PTAB, "   -> L:%u S:%u PTE:%#010lx PHY:%#010llx A:%#05llx TABLE",
                   lev, shift,
                   Buddy::ptr_to_phys (pte),
                   static_cast<uint64>(pte->addr()),
                   static_cast<uint64>(pte->attr()));
        }
    }

    return true;
}

/*
 * PAE note: A processor may prefetch entries into the TLB in the middle
 * of an operation which clears a 64bit PTE. So we must always clear the
 * P-bit in the low word of the PTE first. Since the sequence pte->val = 0;
 * leaves the order of the write dependent on the compiler, it must be coded
 * explicitly as a clear of the low word followed by a clear of the high
 * word. Furthermore, there must be a write memory barrier to enforce proper
 * ordering by the compiler (possibly processor as well).
 */
bool Ptab::unmap (mword virt, size_t size)
{
    while (size) {

        trace (TRACE_PTAB, "(U) %#010lx LIN:%#010lx SIZE:%#x",
               Buddy::ptr_to_phys (this), virt, size);

        unsigned lev = levels;

        for (Ptab *pte = this;; pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr()))) {

            unsigned shift = --lev * bits_per_level + 12;
            pte += virt >> shift & ((1ul << bits_per_level) - 1);

            if (pte->present()) {

                trace (TRACE_PTAB, "   -> L:%u S:%u PTE:%#010lx PHY:%#010llx A:%#05llx %s",
                       lev, shift,
                       Buddy::ptr_to_phys (pte),
                       static_cast<uint64>(pte->addr()),
                       static_cast<uint64>(pte->attr()),
                       lev && !pte->superpage() ? "TABLE" : "FRAME");

                if (lev && !pte->superpage())
                    continue;

                // XXX: use cmpxchg
                pte->val = 0;

                Tlb::flush (virt);
            }

            size_t mask = 1ul << shift;
            size -= mask;
            virt += mask;
            break;
        }
    }

    return true;
}

size_t Ptab::lookup (mword virt, Paddr &phys)
{
    unsigned lev = levels;

    for (Ptab *pte = this;; pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr()))) {

        unsigned shift = --lev * bits_per_level + 12;
        pte += virt >> shift & ((1ul << bits_per_level) - 1);

        if (EXPECT_FALSE (!pte->present()))
            return 0;

        if (EXPECT_FALSE (lev && !pte->superpage()))
            continue;

        size_t size = 1ul << shift;

        phys = pte->addr() | (virt & (size - 1));

        return size;
    }
}

void Ptab::sync_local()
{
    unsigned lev = levels;
    Ptab *pte, *mst;

    for (pte = this, mst = Pd::kern.cpu_ptab();;
         pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr())),
         mst = static_cast<Ptab *>(Buddy::phys_to_ptr (mst->addr()))) {

        unsigned shift = --lev * bits_per_level + 12;
        unsigned slot = LOCAL_SADDR >> shift & ((1ul << bits_per_level) - 1);
        size_t size = 1ul << shift;

        mst += slot;
        assert (mst->present());

        pte += slot;
        assert (!pte->present());

        if (size <= LOCAL_EADDR - LOCAL_SADDR) {
            *pte = *mst;
            break;
        }

        pte->val = Buddy::ptr_to_phys (new Ptab) | (ATTR_PTAB & ptab_bits[lev]);
    }
}

size_t Ptab::sync_master (mword virt)
{
    unsigned lev = levels;
    Ptab *pte, *mst;

    for (pte = this, mst = Ptab::master();;
         pte = static_cast<Ptab *>(Buddy::phys_to_ptr (pte->addr())),
         mst = static_cast<Ptab *>(Buddy::phys_to_ptr (mst->addr()))) {

        unsigned shift = --lev * bits_per_level + 12;
        unsigned slot = virt >> shift & ((1ul << bits_per_level) - 1);
        size_t size = 1ul << shift;

        mst += slot;
        pte += slot;

        if (mst->present()) {

            if (slot == (LOCAL_SADDR >> shift & ((1ul << bits_per_level) - 1))) {
                assert (pte->present());
                continue;
            }

            *pte = *mst;

            trace (TRACE_PTAB, "(S)   L:%u S:%u PTE:%#010lx MST:%#010lx %#010lx %#010llx %#x",
                   lev, shift,
                   Buddy::ptr_to_phys (pte),
                   Buddy::ptr_to_phys (mst),
                   virt & ~(size - 1),
                   static_cast<uint64>(pte->addr()),
                   size);
        }

        return size;
    }
}

void Ptab::sync_master_range (mword s_addr, mword e_addr)
{
    while (s_addr < e_addr) {
        size_t size = sync_master (s_addr);
        s_addr = (s_addr & ~(size - 1)) + size;
    }
}

void *Ptab::remap (Paddr phys)
{
    unsigned offset = static_cast<unsigned>(phys & ((1u << 22) - 1));

    phys &= ~offset;

    if (phys != remap_addr) {
        unmap (REMAP_SADDR, REMAP_EADDR - REMAP_SADDR);
        map (REMAP_SADDR, phys, REMAP_EADDR - REMAP_SADDR, ATTR_WRITABLE);
        remap_addr = phys;
    }

    return reinterpret_cast<void *>(REMAP_SADDR + offset);
}
