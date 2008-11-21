/*
 * Global System Interrupts (GSI)
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

#include "acpi.h"
#include "assert.h"
#include "console_vga.h"
#include "ec.h"
#include "gsi.h"
#include "ioapic.h"
#include "keyb.h"
#include "lapic.h"
#include "pic.h"
#include "state.h"
#include "stdio.h"
#include "wq.h"

Gsi         Gsi::gsi_table[NUM_GSI];
unsigned    Gsi::irq_table[NUM_IRQ];

void Gsi::setup()
{
    for (unsigned i = 0; i < NUM_IRQ; i++) {
        gsi_table[i].cpu = 0;
        gsi_table[i].irt |= MASKED | TRG_EDGE | POL_HIGH | (VEC_GSI + i);
        irq_table[i] = i;
    }

    for (unsigned i = NUM_IRQ; i < NUM_GSI; i++) {
        gsi_table[i].cpu = 0;
        gsi_table[i].irt |= MASKED | TRG_LEVEL | POL_LOW | (VEC_GSI + i);
    }
}

void Gsi::init()
{
    if (Acpi::mode == Acpi::APIC) {
        for (unsigned gsi = 0; gsi < NUM_GSI; gsi++)
            if (gsi_table[gsi].ioapic) {
                gsi_table[gsi].ioapic->set_cpu (gsi_table[gsi].pin, gsi_table[gsi].cpu);
                gsi_table[gsi].ioapic->set_irt (gsi_table[gsi].pin, gsi_table[gsi].irt);
            }
    } else {
        unsigned elcr = 0;
        for (unsigned gsi = 0; gsi < NUM_IRQ; gsi++)
            if (gsi_table[gsi].irt & TRG_LEVEL)
                elcr |= 1u << gsi;
        trace (TRACE_INTERRUPT, "ELCR: %#x", elcr);
        Io::out (ELCR_SLV, static_cast<uint8>(elcr >> 8));
        Io::out (ELCR_MST, static_cast<uint8>(elcr));
    }
}

void Gsi::mask (unsigned gsi)
{
    if (Acpi::mode == Acpi::APIC) {
        assert (gsi < NUM_GSI);
        assert (gsi_table[gsi].ioapic);
        gsi_table[gsi].irt |= MASKED;
        gsi_table[gsi].ioapic->set_irt (gsi_table[gsi].pin, gsi_table[gsi].irt);
    } else {
        assert (gsi < NUM_IRQ);
        (gsi & 8 ? Pic::slave : Pic::master).pin_mask (gsi & 7);
    }

    screen.put (24, 5 + gsi, Console_vga::COLOR_WHITE, 'M');

    trace (TRACE_INTERRUPT, "GSI: %#x masked", gsi);
}

void Gsi::unmask (unsigned gsi)
{
    if (Acpi::mode == Acpi::APIC) {
        assert (gsi < NUM_GSI);
        assert (gsi_table[gsi].ioapic);
        gsi_table[gsi].irt &= ~MASKED;
        gsi_table[gsi].ioapic->set_irt (gsi_table[gsi].pin, gsi_table[gsi].irt);
    } else {
        assert (gsi < NUM_IRQ);
        (gsi & 8 ? Pic::slave : Pic::master).pin_unmask (gsi & 7);
    }

    screen.put (24, 5 + gsi, Console_vga::COLOR_WHITE, 'U');

    trace (TRACE_INTERRUPT, "GSI: %#x unmasked", gsi);
}

void Gsi::eoi (unsigned gsi)
{
    if (Acpi::mode == Acpi::APIC) {
        assert (gsi < NUM_GSI);
        assert (gsi_table[gsi].ioapic);
        Lapic::eoi();
    } else {
        assert (gsi < NUM_IRQ);
        if (gsi & 8)
            Pic::slave.eoi();
        Pic::master.eoi();
    }
}

void Gsi::vector (unsigned vector)
{
    unsigned gsi = vector - VEC_GSI;

    if (gsi == Keyb::gsi)
        Keyb::interrupt();

    if (gsi_table[gsi].irt & TRG_LEVEL)
        mask (gsi);

    eoi (gsi);

    Ec *ec = gsi_table[gsi].wq->send_dequeue();
    if (EXPECT_TRUE (ec))
        ec->release();

    if (EXPECT_FALSE (Cpu::spinner))
        screen.put (Cpu::spinner, 5 + gsi, ec ? Console_vga::COLOR_LIGHT_YELLOW : Console_vga::COLOR_YELLOW, (++Cpu::irqs[gsi] & 0xf)["0123456789ABCDEF"]);
}
