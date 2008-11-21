/*
 * Interrupt Descriptor Table (IDT)
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
#include "types.h"
#include "vectors.h"

class Idt : public Descriptor
{
    private:
        uint32 lo, hi;

        ALWAYS_INLINE
        inline void set (Type type, unsigned dpl, unsigned selector, mword handler)
        {
            lo = selector << 16 | (handler & 0xffff);
            hi = (handler & 0xffff0000) | 1u << 15 | dpl << 13 | type;
        }

    public:
        static Idt idt[VEC_MAX];

        INIT
        static void build();

        ALWAYS_INLINE
        static inline void load()
        {
            Pseudo_descriptor desc (sizeof (idt) - 1, reinterpret_cast<mword>(idt));
            asm volatile ("lidt %0" : : "m" (desc));
        }
};
