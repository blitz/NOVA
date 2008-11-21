/*
 * Central Processing Unit (CPU)
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
#include "bits.h"
#include "cmdline.h"
#include "cpu.h"
#include "extern.h"
#include "gdt.h"
#include "hip.h"
#include "idt.h"
#include "lapic.h"
#include "memory.h"
#include "msr.h"
#include "pd.h"
#include "ptab.h"
#include "selectors.h"
#include "stdio.h"
#include "tss.h"
#include "vmx.h"

char const * const Cpu::vendor_string[] =
{
    "Unknown",
    "GenuineIntel",
    "AuthenticAMD"
};

unsigned    Cpu::boot_lock;
unsigned    Cpu::booted;
unsigned    Cpu::secure;

// Order of these matters
unsigned    Cpu::id;
unsigned    Cpu::package;
unsigned    Cpu::core;
unsigned    Cpu::thread;

Cpu::Vendor Cpu::vendor;
unsigned    Cpu::platform;
unsigned    Cpu::family;
unsigned    Cpu::model;
unsigned    Cpu::stepping;
unsigned    Cpu::brand;
unsigned    Cpu::patch;

uint32      Cpu::name[12];
uint32      Cpu::features[4];
bool        Cpu::bsp;

// Just for debugging
unsigned    Cpu::spinner;
unsigned    Cpu::vtlb_miss_count;
unsigned    Cpu::vtlb_refl_count;
unsigned    Cpu::vtlb_flush_count;
unsigned    Cpu::apic_timer_count;
unsigned    Cpu::irqs[NUM_GSI];
unsigned    Cpu::exit[64];

void Cpu::wakeup_ap()
{
    for (unsigned i = 0; i < 256; i++) {

        if (!Lapic::chk_present (i))
            continue;

        if (Lapic::id() == i)
            continue;

        Lapic::send_ipi (i, Lapic::DST_PHYSICAL, Lapic::DLV_INIT, 0);
        Acpi::delay (10);
        Lapic::send_ipi (i, Lapic::DST_PHYSICAL, Lapic::DLV_SIPI, 1);
        Acpi::delay (1);
        Lapic::send_ipi (i, Lapic::DST_PHYSICAL, Lapic::DLV_SIPI, 1);
    }
}

void Cpu::check_features()
{
    unsigned tpp = 1;
    unsigned cpp = 1;

    uint32 eax, ebx, ecx, edx;

    cpuid (0, eax, ebx, ecx, edx);

    unsigned v;
    for (v = sizeof (vendor_string) / sizeof (*vendor_string); --v;)
        if (*reinterpret_cast<uint32 const *>(vendor_string[v] + 0) == ebx &&
            *reinterpret_cast<uint32 const *>(vendor_string[v] + 4) == edx &&
            *reinterpret_cast<uint32 const *>(vendor_string[v] + 8) == ecx)
            break;

    vendor = Vendor (v);

    if (vendor == INTEL) {
        Msr::write<uint64>(Msr::IA32_BIOS_SIGN_ID, 0);
        platform = static_cast<unsigned>(Msr::read<uint64>(Msr::IA32_PLATFORM_ID) >> 50) & 7;
    }

    switch (static_cast<uint8>(eax)) {
        default:
            cpuid (0x4, 0, eax, ebx, ecx, edx);
            cpp = (eax >> 26 & 0x3f) + 1;
        case 0x1 ... 0x3:
            cpuid (0x1, eax, ebx, features[1], features[0]);
            family   = (eax >> 8 & 0xf) + (eax >> 20 & 0xff);
            model    = (eax >> 4 & 0xf) + (eax >> 12 & 0xf0);
            stepping =  eax & 0xf;
            brand    =  ebx & 0xff;
            id       =  ebx >> 24;
            tpp      =  ebx >> 16 & 0xff;
    }

    patch = static_cast<unsigned>(Msr::read<uint64>(Msr::IA32_BIOS_SIGN_ID) >> 32);

    cpuid (0x80000000, eax, ebx, ecx, edx);

    switch (static_cast<uint8>(eax)) {
        default:
            if (vendor == AMD) {
                cpuid (0x80000005, eax, ebx, ecx, edx);
                trace (TRACE_CPU, "L1D: C:%luK/%lu/%lu/%lu 4k:%lu/%lx 2M:%lu/%lx  L1I: C:%luK/%lu/%lu/%lu 4k:%lu/%lx 2M:%lu/%lx",
                       ecx >> 24, ecx >> 16 & 0xff, ecx >> 8 & 0xff, ecx & 0xff,
                       ebx >> 16 & 0xff, ebx >> 24, eax >> 16 & 0xff, eax >> 24,
                       edx >> 24, edx >> 16 & 0xff, edx >> 8 & 0xff, edx & 0xff,
                       ebx & 0xff, ebx >> 8 & 0xff, eax & 0xff, eax >> 8 & 0xff);
            }
        case 4:
            cpuid (0x80000004, name[8], name[9], name[10], name[11]);
        case 3:
            cpuid (0x80000003, name[4], name[5], name[6], name[7]);
        case 2:
            cpuid (0x80000002, name[0], name[1], name[2], name[3]);
        case 1:
            cpuid (0x80000001, eax, ebx, features[3], features[2]);
    }

    if (!feature (FEAT_HTT))
        id = booted;
    else if (feature (FEAT_CMP_LEGACY))
        cpp = tpp;

    unsigned tpc = tpp / cpp;
    unsigned t_bits = bit_scan_reverse (tpc - 1) + 1;
    unsigned c_bits = bit_scan_reverse (cpp - 1) + 1;

    thread  = id            & ((1u << t_bits) - 1);
    core    = id >>  t_bits & ((1u << c_bits) - 1);
    package = id >> (t_bits + c_bits);

    booted++;

    if (vendor == AMD && model >= 0x40 && cpp > 1)
        Msr::write (Msr::AMD_IPMR, Msr::read<uint32>(Msr::AMD_IPMR) & ~(3ul << 27));
}

void Cpu::setup_thermal()
{
    Msr::write (Msr::IA32_THERM_INTERRUPT, 0x1d);
}

void Cpu::setup_sysenter()
{
    Msr::write<mword>(Msr::IA32_SYSENTER_CS,  SEL_KERN_CODE);
    Msr::write<mword>(Msr::IA32_SYSENTER_ESP, reinterpret_cast<mword>(&Tss::run.esp0));
    Msr::write<mword>(Msr::IA32_SYSENTER_EIP, reinterpret_cast<mword>(&entry_sysenter));
}

void Cpu::init()
{
    Gdt::build();
    Tss::build();

    Gdt::load();
    Tss::load();
    Idt::load();

    check_features();

    if (!Cmdline::nospinner)
        spinner = init_spinner (id);

    Pd::init_cpu_ptab (Ptab::current());

    Lapic::init();

    if (EXPECT_TRUE (feature (FEAT_ACPI)))
        setup_thermal();

    if (EXPECT_TRUE (feature (FEAT_SEP)))
        setup_sysenter();

    if (EXPECT_TRUE (feature (FEAT_VMX)))
        Vmcs::init();

    trace (TRACE_CPU, "CPU: %x:%x:%x:%x [%x] (%u:%u:%u) %.48s",
           family, model, stepping, platform, patch,
           package, core, thread,
           reinterpret_cast<char *>(name));

    Hip::add_cpu();

    if (bsp && !secure)
        wakeup_ap();

    boot_lock++;
}
