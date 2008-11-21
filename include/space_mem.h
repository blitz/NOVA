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

#pragma once

#include "compiler.h"
#include "config.h"
#include "cpu.h"
#include "ptab.h"
#include "types.h"

class Exc_regs;

class Space_mem
{
    protected:
        Ptab *master;
        Ptab *percpu[NUM_CPU];

    public:
        static Mr array[0x100000];

        // Constructor
        Space_mem();

        // Constructor for Pd::kern memory space
        Space_mem (Ptab *mst) : master (mst) {}

        ALWAYS_INLINE
        inline Ptab *mst_ptab() const
        {
            return master;
        }

        ALWAYS_INLINE
        inline Ptab *cpu_ptab() const
        {
            return percpu[Cpu::id];
        }

        ALWAYS_INLINE
        inline void mem_map_local (mword virt, Paddr phys, size_t size, Paging::Attribute attr)
        {
            percpu[Cpu::id]->map (virt, phys, size, attr);
        }

        ALWAYS_INLINE
        inline size_t lookup (mword virt, Paddr &phys)
        {
            return master->lookup (virt, phys);
        }

        ALWAYS_INLINE
        inline bool insert (mword virt, Paddr phys, size_t size, Paging::Attribute attr)
        {
            return master->map (virt, phys, size, attr);
        }

        ALWAYS_INLINE
        inline bool remove (mword virt, size_t size)
        {
            return master->unmap (virt, size);
        }

        static void page_fault (Exc_regs *regs);
};
