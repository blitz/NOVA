/*
 * I/O Advanced Programmable Interrupt Controller (I/O APIC)
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

#include "apic.h"
#include "compiler.h"
#include "ptab.h"
#include "types.h"

class Ioapic : public Apic
{
    private:
        enum
        {
            IOAPIC_IDX  = 0x0,          // Index Register           r/w, 8
            IOAPIC_WND  = 0x10,         // Window Register          r/w, 32
            IOAPIC_PAR  = 0x20,         // Pin Assertion Register   w/o, 8
            IOAPIC_EOI  = 0x40,         // EOI Register             w/o, 8
        };

        enum Register
        {
            IOAPIC_ID   = 0x0,          // APIC ID Register         r/w, 32
            IOAPIC_VER  = 0x1,          // Version Register         r/o, 32
            IOAPIC_ARB  = 0x2,          // Arbitration ID Register  r/o, 32
            IOAPIC_BCFG = 0x3,          // Boot Config Register     r/w, 32
            IOAPIC_IRT  = 0x10,         // r/w, 32+32
        };

        ALWAYS_INLINE
        inline void index (Register reg)
        {
            *reinterpret_cast<uint8 volatile *>(reinterpret_cast<mword>(this) + IOAPIC_IDX) = reg;
        }

        ALWAYS_INLINE
        inline uint32 read (Register reg)
        {
            index (reg);
            return *reinterpret_cast<uint32 volatile *>(reinterpret_cast<mword>(this) + IOAPIC_WND);
        }

        ALWAYS_INLINE
        inline void write (Register reg, uint32 val)
        {
            index (reg);
            *reinterpret_cast<uint32 volatile *>(reinterpret_cast<mword>(this) + IOAPIC_WND) = val;
        }

    public:
        Ioapic();

        ALWAYS_INLINE
        static inline void *operator new (size_t, Paddr phys)
        {
            extern mword hwdev_addr;

            Ptab::master()->map (hwdev_addr -= PAGE_SIZE,
                                 phys & ~PAGE_MASK,
                                 PAGE_SIZE,
                                 Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                                  Ptab::ATTR_GLOBAL      |
                                                  Ptab::ATTR_UNCACHEABLE |
                                                  Ptab::ATTR_WRITABLE));

            return reinterpret_cast<void *>(hwdev_addr | (phys & PAGE_MASK));
        }

        ALWAYS_INLINE
        inline unsigned id()
        {
            return read (IOAPIC_ID) >> 24 & 0xf;
        }

        ALWAYS_INLINE
        inline unsigned version()
        {
            return read (IOAPIC_VER) & 0xff;
        }

        ALWAYS_INLINE
        inline unsigned prq()
        {
            return read (IOAPIC_VER) >> 15 & 0x1;
        }

        ALWAYS_INLINE
        inline unsigned irt_max()
        {
            return read (IOAPIC_VER) >> 16 & 0xff;
        }

        ALWAYS_INLINE
        inline void set_irt (unsigned pin, unsigned val)
        {
            write (Register (IOAPIC_IRT + 2 * pin), val);
        }

        ALWAYS_INLINE
        inline void set_cpu (unsigned pin, unsigned cpu)
        {
            write (Register (IOAPIC_IRT + 2 * pin + 1), cpu << 24);
        }
};
