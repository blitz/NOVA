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

#pragma once

#include "compiler.h"
#include "mdb.h"
#include "types.h"

class Exc_regs;
class Space_mem;

class Space_io
{
    private:
        Space_mem *space_mem;

        ALWAYS_INLINE
        static inline mword idx_to_virt (unsigned idx)
        {
            return IOBMP_SADDR + (idx / 8 / sizeof (mword)) * sizeof (mword);
        }

        ALWAYS_INLINE
        static inline unsigned idx_to_bit (unsigned idx)
        {
            return idx % (8 * sizeof (mword));
        }

    public:
        static Mr array[0x10000];

        ALWAYS_INLINE
        inline Space_io (Space_mem *space) : space_mem (space) {}

        ALWAYS_INLINE
        static inline bool lookup (unsigned idx)
        {
            return !(idx_to_virt (idx) & 1ul << idx_to_bit (idx));
        }

        bool insert (unsigned);
        bool remove (unsigned);

        static void page_fault (Exc_regs *);
};
