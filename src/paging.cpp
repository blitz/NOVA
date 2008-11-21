/*
 * IA32 Paging Support
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

#include "cpu.h"
#include "msr.h"
#include "paging.h"

Paging::Attribute Paging::ptab_bits[levels];

void Paging::check_features()
{
    uint32 eax, ebx, ecx, edx;
    Cpu::cpuid (0x80000001, eax, ebx, ecx, edx);
    Attribute nx = edx & 1ul << 20 ? ATTR_NONE : ATTR_NOEXEC;

    extern Attribute ptab_bits_p[];
    switch (levels) {
        case 3:
            ptab_bits_p[2] = Attribute (~ATTR_NOEXEC & ~ATTR_ACCESSED & ~ATTR_USER & ~ATTR_WRITABLE);
        case 2:
            ptab_bits_p[1] = Attribute (~nx);
        case 1:
            ptab_bits_p[0] = Attribute (~nx);
    }
}

void Paging::enable()
{
    extern Attribute ptab_bits_p;
    if (ptab_bits_p & ATTR_NOEXEC)
        Msr::write<uint32>(Msr::IA32_EFER, 1ul << 11);

    extern char _master_p;
    asm volatile ("mov %0, %%cr3" : : "r" (&_master_p));

#ifdef PAE
    Cpu::set_cr4 (Cpu::get_cr4() | Cpu::CR4_PGE | Cpu::CR4_PAE | Cpu::CR4_DE);
#else
    Cpu::set_cr4 (Cpu::get_cr4() | Cpu::CR4_PGE | Cpu::CR4_PSE | Cpu::CR4_DE);
#endif

    Cpu::set_cr0 (Cpu::get_cr0() | Cpu::CR0_PG | Cpu::CR0_AM | Cpu::CR0_WP | Cpu::CR0_NE | Cpu::CR0_MP | Cpu::CR0_PE);
}
