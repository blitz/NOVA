/*
 * Local Advanced Programmable Interrupt Controller (Local APIC)
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
#include "console_vga.h"
#include "cpu.h"
#include "lapic.h"
#include "pd.h"
#include "msr.h"
#include "sc.h"
#include "stdio.h"
#include "vectors.h"

unsigned    Lapic::fsb_speed;
uint32      Lapic::present[8];

void Lapic::init()
{
    Paddr apic_base = Msr::read<Paddr>(Msr::IA32_APIC_BASE);
    if (!(apic_base & 0x800))
        Msr::write (Msr::IA32_APIC_BASE, apic_base | 0x800);

    Cpu::bsp = apic_base & 0x100;

    Pd::kern.mem_map_local (LAPIC_SADDR,
                            apic_base & ~PAGE_MASK,
                            LAPIC_EADDR - LAPIC_SADDR,
                            Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                             Ptab::ATTR_GLOBAL      |
                                             Ptab::ATTR_UNCACHEABLE |
                                             Ptab::ATTR_WRITABLE));

    uint32 svr = read (LAPIC_SVR);
    if (!(svr & 0x100))
        write (LAPIC_SVR, svr | 0x100);

    switch (lvt_max()) {
        default:
            set_lvt (LAPIC_LVT_THERM, VEC_LVT_THERM, DLV_FIXED);
        case 4:
            set_lvt (LAPIC_LVT_PERFM, VEC_LVT_PERFM, DLV_FIXED);
        case 3:
            set_lvt (LAPIC_LVT_ERROR, VEC_LVT_ERROR, DLV_FIXED);
        case 2:
            set_lvt (LAPIC_LVT_LINT1, 0, DLV_NMI);
        case 1:
            set_lvt (LAPIC_LVT_LINT0, 0, DLV_EXTINT, Cpu::bsp && Acpi::mode == Acpi::PIC ? UNMASKED : MASKED);
        case 0:
            set_lvt (LAPIC_LVT_TIMER, VEC_LVT_TIMER, DLV_FIXED, UNMASKED, PERIODIC);
    }

    set_timer_dcr (DIVIDE_BY_1);
    set_timer_icr (~0ul);

    set_tpr (0);
    set_ldr (1u << Cpu::id);
    set_dfr (LAPIC_FLAT);

    if (Cpu::bsp)
        calibrate();

    set_timer_icr (fsb_speed / 100);

    trace (TRACE_APIC, "LAPIC: LIN:%#x PHY:%#llx ID:%#x VER:%#x LVT:%#x",
           LAPIC_SADDR, static_cast<uint64>(apic_base & ~PAGE_MASK),
           id(), version(), lvt_max());
}

void Lapic::calibrate()
{
    uint32 v1 = timer_ccr();
    Acpi::delay (250);
    uint32 v2 = timer_ccr();

    fsb_speed = (v1 - v2) * 4;

    trace (TRACE_CPU, "FSB: %u Hz", fsb_speed);
}

void Lapic::send_ipi (unsigned cpu, Destination_mode dst, Delivery_mode dlv, unsigned vector)
{
    while (EXPECT_FALSE (read (LAPIC_ICR_LO) & STS_PENDING))
        Cpu::pause();

    write (LAPIC_ICR_HI, cpu << 24);
    write (LAPIC_ICR_LO, dst | dlv | vector);
}

void Lapic::therm_handler() {}

void Lapic::perfm_handler() {}

void Lapic::error_handler()
{
    write (LAPIC_ESR, 0);
    write (LAPIC_ESR, 0);
}

void Lapic::timer_handler()
{
    Sc::timer_tick();

    if (EXPECT_FALSE (Cpu::spinner))
        screen.put (Cpu::spinner, 1, Console_vga::COLOR_LIGHT_BLUE, (++Cpu::apic_timer_count & 0xf)["0123456789ABCDEF"]);
}

void Lapic::vector (unsigned vector)
{
    unsigned lvt = vector - VEC_LVT;

    if (EXPECT_TRUE (lvt == 0))
        timer_handler();

    else if (EXPECT_TRUE (lvt == 3))
        error_handler();

    else if (EXPECT_TRUE (lvt == 4))
        perfm_handler();

    else if (EXPECT_TRUE (lvt == 5))
        therm_handler();

    eoi();
}
