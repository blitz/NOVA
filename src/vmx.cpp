/*
 * Virtual Machine Extensions (VMX)
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

#include "cpu.h"
#include "extern.h"
#include "gdt.h"
#include "idt.h"
#include "msr.h"
#include "stdio.h"
#include "tss.h"
#include "vmx.h"

Vmcs *      Vmcs::current;
uint64      Vmcs::ctrl_pin;
uint64      Vmcs::ctrl_cpu;
uint64      Vmcs::ctrl_exi;
uint64      Vmcs::ctrl_ent;
uint32      Vmcs::revision;
mword       Vmcs::cr0_fix0;
mword       Vmcs::cr0_fix1;
mword       Vmcs::cr4_fix0;
mword       Vmcs::cr4_fix1;

Vmcs::Vmcs (mword guest_cr3, mword host_cr3) : rev (revision)
{
    clear();

    make_current();

    uint32 pin = EXIT_EXTINT | EXIT_NMI;
    pin |= static_cast<uint32>(ctrl_pin);
    pin &= static_cast<uint32>(ctrl_pin >> 32);
    write (PIN_EXEC_CTRL, pin);

    uint32 cpu = 0;
    cpu |= static_cast<uint32>(ctrl_cpu);
    cpu &= static_cast<uint32>(ctrl_cpu >> 32);
    write (CPU_EXEC_CTRL_1, cpu);

    uint32 exi = INTA_ON_EXIT;
    exi |= static_cast<uint32>(ctrl_exi);
    exi &= static_cast<uint32>(ctrl_exi >> 32);
    write (EXI_CONTROLS, exi);

    uint32 ent = 0;
    ent |= static_cast<uint32>(ctrl_ent);
    ent &= static_cast<uint32>(ctrl_ent >> 32);
    write (ENT_CONTROLS, ent);

    write (EXC_BITMAP, 0x4000);
    write (PF_ERROR_MASK, 0);
    write (PF_ERROR_MATCH, 0);

    write (CR0_MASK, Vmcs::cr0_fix0 | ~Vmcs::cr0_fix1);
    write (CR4_MASK, Vmcs::cr4_fix0 | ~Vmcs::cr4_fix1);
    write (CR3_TARGET_COUNT, 0);

    write (VMCS_LINK_PTR,    ~0ul);
    write (VMCS_LINK_PTR_HI, ~0ul);

    write (GUEST_CR3, guest_cr3);

    write (HOST_SEL_CS, SEL_KERN_CODE);
    write (HOST_SEL_SS, SEL_KERN_DATA);
    write (HOST_SEL_DS, SEL_KERN_DATA);
    write (HOST_SEL_ES, SEL_KERN_DATA);
    write (HOST_SEL_TR, SEL_TSS_RUN);

    write (HOST_CR0, Cpu::get_cr0());
    write (HOST_CR3, host_cr3);
    write (HOST_CR4, Cpu::get_cr4());

    write (HOST_BASE_TR,   reinterpret_cast<mword>(&Tss::run));
    write (HOST_BASE_GDTR, reinterpret_cast<mword>(Gdt::gdt));
    write (HOST_BASE_IDTR, reinterpret_cast<mword>(Idt::idt));

    write (HOST_SYSENTER_CS,  SEL_KERN_CODE);
    write (HOST_SYSENTER_ESP, reinterpret_cast<mword>(&Tss::run.esp0));
    write (HOST_SYSENTER_EIP, reinterpret_cast<mword>(&entry_sysenter));

    write (HOST_RSP, reinterpret_cast<mword>(&Tss::run.esp0));
    write (HOST_RIP, reinterpret_cast<mword>(&entry_vmexit));
}

void Vmcs::init()
{
    unsigned bits = Cpu::secure ? 0x3 : 0x5;
    if ((Msr::read<uint32>(Msr::IA32_FEATURE_CONTROL) & bits) != bits)
        return;

    mword const cr4_hv0 = Cpu::CR4_PSE;
    mword const cr4_hv1 = Cpu::CR4_PGE | Cpu::CR4_PAE;
    mword const cr0_hv0 = Cpu::CR0_PG | Cpu::CR0_WP | Cpu::CR0_PE;

    cr4_fix0 = Msr::read<mword>(Msr::IA32_VMX_CR4_FIXED0) |  cr4_hv0;
    cr4_fix1 = Msr::read<mword>(Msr::IA32_VMX_CR4_FIXED1) & ~cr4_hv1;
    cr0_fix0 = Msr::read<mword>(Msr::IA32_VMX_CR0_FIXED0) |  cr0_hv0;
    cr0_fix1 = Msr::read<mword>(Msr::IA32_VMX_CR0_FIXED1);

    ctrl_pin = Msr::read<uint64>(Msr::IA32_VMX_CTLS_PIN);
    ctrl_cpu = Msr::read<uint64>(Msr::IA32_VMX_CTLS_CPU_1);
    ctrl_exi = Msr::read<uint64>(Msr::IA32_VMX_CTLS_EXIT);
    ctrl_ent = Msr::read<uint64>(Msr::IA32_VMX_CTLS_ENTRY);
    revision = Msr::read<uint32>(Msr::IA32_VMX_BASIC);

    Cpu::set_cr4 ((Cpu::get_cr4() | cr4_fix0) & cr4_fix1);
    Cpu::set_cr0 ((Cpu::get_cr0() | cr0_fix0) & cr0_fix1);

    Vmcs *root = new Vmcs;

    trace (TRACE_VMX, "VMCS: PHY:%#010lx Rev:%#lx CPU:%#llx", Buddy::ptr_to_phys (root), revision, ctrl_cpu);
}
