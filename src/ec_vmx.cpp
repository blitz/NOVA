/*
 * Execution Context
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

#include "ec.h"
#include "gdt.h"
#include "gsi.h"
#include "lapic.h"
#include "vmx.h"
#include "vtlb.h"

void Ec::vmx_exception()
{
    uint32 intr_info  = Vmcs::read (Vmcs::EXI_INTR_INFO);
    uint32 intr_error = Vmcs::read (Vmcs::EXI_INTR_ERROR);
    assert (intr_info & 0x80000000);

    // Handle event during IDT vectoring
    uint32 vect_info  = Vmcs::read (Vmcs::IDT_VECT_INFO);
    uint32 vect_error = Vmcs::read (Vmcs::IDT_VECT_ERROR);

    if (vect_info & 0x80000000) {
        Vmcs::write (Vmcs::ENT_INTR_INFO,  vect_info);
        Vmcs::write (Vmcs::ENT_INTR_ERROR, vect_error);
        switch (vect_info >> 8 & 0x7) {
            case 4 ... 6:
                Vmcs::write (Vmcs::ENT_INST_LEN, Vmcs::read (Vmcs::EXI_INST_LEN));
                break;
        }
    };

    current->regs.vec = Vmcs::VMX_EXCEPTION;

    if ((intr_info >> 8 & 0x7) == 0x3 && (intr_info & 0xff) == 0xe) {
        mword addr = Vmcs::read (Vmcs::EXI_QUALIFICATION);

        switch (Vtlb::miss (&current->regs, addr, intr_error)) {

            case Vtlb::GPA_HPA:
                current->regs.ept_fault = addr;
                current->regs.vec = Vmcs::VMX_EPT_VIOLATION;
                break;

            case Vtlb::GLA_GPA:
                current->regs.cr2 = addr;
                Vmcs::write (Vmcs::ENT_INTR_INFO,  intr_info);
                Vmcs::write (Vmcs::ENT_INTR_ERROR, intr_error);

                // Output Spinner
                if (EXPECT_FALSE (Cpu::spinner))
                    screen.put (Cpu::spinner, 2, Console_vga::COLOR_LIGHT_CYAN, (++Cpu::vtlb_refl_count & 0xf)["0123456789ABCDEF"]);

                ret_user_vmresume();

            case Vtlb::SUCCESS:
                // Output Spinner
                if (EXPECT_FALSE (Cpu::spinner))
                    screen.put (Cpu::spinner, 3, Console_vga::COLOR_LIGHT_MAGENTA, (++Cpu::vtlb_miss_count & 0xf)["0123456789ABCDEF"]);

                ret_user_vmresume();
        }
    }

    send_vmx_msg();
}

void Ec::vmx_extint()
{
    unsigned vector = Vmcs::read (Vmcs::EXI_INTR_INFO) & 0xff;

    if (vector >= VEC_LVT)
        Lapic::vector (vector);
    else if (vector >= VEC_GSI)
        Gsi::vector (vector);

    current->regs.vec = Vmcs::VMX_EXTINT;
    current->continuation = send_vmx_msg;

    Sc::schedule();
}

void Ec::vmx_handler()
{
    mword reason = Vmcs::read (Vmcs::EXI_REASON) & 0xff;

    Cpu::exit[reason]++;

    switch (reason) {
        case Vmcs::VMX_EXCEPTION:
            vmx_exception();
            break;
        case Vmcs::VMX_EXTINT:
            vmx_extint();
            break;
    }

    current->regs.vec = reason;
    send_vmx_msg();
}

void Ec::ret_user_vmresume()
{
    Gdt::gdt[SEL_TSS_RUN >> 3].unbusy_tss();

    current->vmcs->make_current();

    asm volatile ("mov %0, %%esp;"
                  "mov %1, %%cr2;"
                  "popa;"
                  "vmresume;"
                  "vmlaunch;"
                  "pusha;"
                  "mov %2, %%esp;"
                  : : "g" (&current->regs), "r" (current->regs.cr2), "i" (KSTCK_EADDR) : "memory");

    current->kill (&current->regs, "VMENTRY");
}
