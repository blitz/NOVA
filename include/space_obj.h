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

#pragma once

#include "capability.h"
#include "compiler.h"
#include "types.h"

class Exc_regs;
class Space_mem;

class Space_obj
{
    private:
        Space_mem *space_mem;

        ALWAYS_INLINE
        static inline mword idx_to_virt (unsigned idx)
        {
            return OBJSP_SADDR + (idx % max) * sizeof (Capability);
        }

    public:
        static unsigned const max = (OBJSP_EADDR - OBJSP_SADDR) / sizeof (Capability);

        ALWAYS_INLINE
        inline Space_obj (Space_mem *space) : space_mem (space) {}

        ALWAYS_INLINE
        static inline Capability lookup (unsigned idx)
        {
            return *reinterpret_cast<Capability *>(idx_to_virt (idx));
        }

        bool insert (unsigned, Capability);
        bool remove (unsigned, Capability);

        static void page_fault (Exc_regs *);
};
