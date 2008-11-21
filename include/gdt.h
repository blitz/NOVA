/*
 * Global Descriptor Table (GDT)
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

#pragma once

#include "compiler.h"
#include "descriptor.h"
#include "selectors.h"
#include "types.h"

class Gdt : public Descriptor
{
    private:
        uint32 lo, hi;

        ALWAYS_INLINE
        inline void set (Type type, Granularity gran, Size size, unsigned dpl, mword base, size_t limit)
        {
            lo = base << 16 | (limit & 0xffff);
            hi = (base & 0xff000000) | gran | size | (limit & 0xf0000) | 1u << 15 | dpl << 13 | type | (base >> 16 & 0xff);
        }

    public:
        static Gdt gdt[SEL_MAX >> 3] CPULOCAL_HOT;

        static void build();

        ALWAYS_INLINE
        static inline void load()
        {
            Pseudo_descriptor desc (sizeof (gdt) - 1, reinterpret_cast<mword>(gdt));
            asm volatile ("lgdt %0" : : "m" (desc));
        }

        ALWAYS_INLINE
        static inline void unbusy_tss()
        {
            gdt[SEL_TSS_RUN >> 3].hi &= ~0x200;
        }
};
