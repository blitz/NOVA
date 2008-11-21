/*
 * Local Advanced Programmable Interrupt Controller (Local APIC)
 *
 * Copyright (C) 2005-2008, Udo Steinberg <udo@hypervisor.org>
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
#include "memory.h"
#include "types.h"

class Lapic : public Apic
{
    private:
        static uint32 present[8];

        enum Register
        {
            LAPIC_IDR       = 0x2,
            LAPIC_LVR       = 0x3,
            LAPIC_TPR       = 0x8,
            LAPIC_PPR       = 0xa,
            LAPIC_EOI       = 0xb,
            LAPIC_LDR       = 0xd,
            LAPIC_DFR       = 0xe,
            LAPIC_SVR       = 0xf,
            LAPIC_ISR       = 0x10,
            LAPIC_TMR       = 0x18,
            LAPIC_IRR       = 0x20,
            LAPIC_ESR       = 0x28,
            LAPIC_ICR_LO    = 0x30,
            LAPIC_ICR_HI    = 0x31,
            LAPIC_LVT_TIMER = 0x32,
            LAPIC_LVT_THERM = 0x33,
            LAPIC_LVT_PERFM = 0x34,
            LAPIC_LVT_LINT0 = 0x35,
            LAPIC_LVT_LINT1 = 0x36,
            LAPIC_LVT_ERROR = 0x37,
            LAPIC_TMR_ICR   = 0x38,
            LAPIC_TMR_CCR   = 0x39,
            LAPIC_TMR_DCR   = 0x3e,
            LAPIC_IPI_SELF  = 0x40
        };

        enum Model
        {
            LAPIC_CLUSTER   = 0x0,
            LAPIC_FLAT      = 0xf
        };

        enum Divider
        {
            DIVIDE_BY_2     = 0,
            DIVIDE_BY_4     = 1,
            DIVIDE_BY_8     = 2,
            DIVIDE_BY_16    = 3,
            DIVIDE_BY_32    = 8,
            DIVIDE_BY_64    = 9,
            DIVIDE_BY_128   = 10,
            DIVIDE_BY_1     = 11
        };

        ALWAYS_INLINE
        static inline uint32 read (Register reg)
        {
            return *reinterpret_cast<uint32 volatile *>(LAPIC_SADDR + (reg << 4));
        }

        ALWAYS_INLINE
        static inline void write (Register reg, uint32 val)
        {
            *reinterpret_cast<uint32 volatile *>(LAPIC_SADDR + (reg << 4)) = val;
        }

        ALWAYS_INLINE
        static inline void set_tpr (unsigned tpr)
        {
            write (LAPIC_TPR, tpr & 0xff);
        }

        ALWAYS_INLINE
        static inline void set_ldr (unsigned id)
        {
            write (LAPIC_LDR, id << 24);
        }

        ALWAYS_INLINE
        static inline void set_dfr (Model model)
        {
            write (LAPIC_DFR, model << 28);
        }

        ALWAYS_INLINE
        static inline void set_lvt (Register reg, unsigned vector, Delivery_mode dlv, Mask msk = UNMASKED, Mode mod = ONESHOT)
        {
            write (reg, mod | msk | dlv | vector);
        }

        ALWAYS_INLINE
        static inline void set_timer_icr (uint32 val)
        {
            write (LAPIC_TMR_ICR, val);
        }

        ALWAYS_INLINE
        static inline void set_timer_dcr (Divider val)
        {
            write (LAPIC_TMR_DCR, val);
        }

        ALWAYS_INLINE
        static inline void timer_handler();

        ALWAYS_INLINE
        static inline void error_handler();

        ALWAYS_INLINE
        static inline void perfm_handler();

        ALWAYS_INLINE
        static inline void therm_handler();

    public:
        static unsigned fsb_speed;

        ALWAYS_INLINE
        static inline void set_present (unsigned id)
        {
            present[id / 32] |= 1u << id % 32;
        }

        ALWAYS_INLINE
        static inline bool chk_present (unsigned id)
        {
            return present[id / 32] & 1u << id % 32;
        }

        ALWAYS_INLINE
        static inline void set_id (unsigned id)
        {
            write (LAPIC_IDR, id << 24);
        }

        ALWAYS_INLINE
        static inline unsigned id()
        {
            return read (LAPIC_IDR) >> 24 & 0xff;
        }

        ALWAYS_INLINE
        static inline unsigned version()
        {
            return read (LAPIC_LVR) & 0xff;
        }

        ALWAYS_INLINE
        static inline unsigned lvt_max()
        {
            return read (LAPIC_LVR) >> 16 & 0xff;
        }

        ALWAYS_INLINE
        static inline void eoi()
        {
            write (LAPIC_EOI, 0);
        }

        ALWAYS_INLINE
        static inline uint32 timer_ccr()
        {
            return read (LAPIC_TMR_CCR);
        }

        static void init();

        static void calibrate();

        static void send_ipi (unsigned cpu, Destination_mode dst, Delivery_mode dlv, unsigned vector);

        REGPARM (1)
        static void vector (unsigned) asm ("lvt_vector");
};
