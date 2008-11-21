/*
 * User Thread Control Block (UTCB)
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

#include "mtd.h"
#include "state.h"
#include "utcb.h"
#include "vmx.h"
#include "vtlb.h"

void Utcb::load (Exc_regs *regs, Mtd mtd)
{
    mtr = mtd;

    if (mtd.xfer (Mtd::GPR_ACDB)) {
        eax = regs->eax;
        ecx = regs->ecx;
        edx = regs->edx;
        ebx = regs->ebx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        ebp = regs->ebp;
        esi = regs->esi;
        edi = regs->edi;
    }

    if (mtd.xfer (Mtd::RSP))
        esp = regs->esp;

    if (mtd.xfer (Mtd::RIP_LEN))
        eip = regs->eip;

    if (mtd.xfer (Mtd::RFLAGS))
        efl = regs->efl;

    if (mtd.xfer (Mtd::DS_ES)) {
        ds_sel = regs->ds;
        es_sel = regs->es;
    }

    if (mtd.xfer (Mtd::FS_GS)) {
        fs_sel = regs->fs;
        gs_sel = regs->gs;
    }

    if (mtd.xfer (Mtd::QUAL))
        qual = regs->err;

    if (mtd.xfer (Mtd::ADDR))
        addr = regs->cr2;
}

mword *Utcb::save (Exc_regs *regs, Mtd mtd)
{
    if (mtd.xfer (Mtd::GPR_ACDB)) {
        regs->eax = eax;
        regs->ecx = ecx;
        regs->edx = edx;
        regs->ebx = ebx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        regs->ebp = ebp;
        regs->esi = esi;
        regs->edi = edi;
    }

    if (mtd.xfer (Mtd::RIP_LEN))
        regs->eip = eip;
    if (mtd.xfer (Mtd::RSP))
        regs->esp = esp;
    if (mtd.xfer (Mtd::RFLAGS))
        regs->efl = efl;

    if (mtd.xfer (Mtd::DS_ES)) {
        regs->ds = ds_sel;
        regs->es = es_sel;
    }

    if (mtd.xfer (Mtd::FS_GS)) {
        regs->fs = fs_sel;
        regs->gs = gs_sel;
    }

    return item;
}

void Utcb::load (Exc_regs *regs, Vmcs *vmcs, Mtd mtd)
{
    mtr = mtd;

    if (mtd.xfer (Mtd::GPR_ACDB)) {
        eax = regs->eax;
        ecx = regs->ecx;
        edx = regs->edx;
        ebx = regs->ebx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        ebp = regs->ebp;
        esi = regs->esi;
        edi = regs->edi;
    }

    vmcs->make_current();

    if (mtd.xfer (Mtd::RSP))
        esp = Vmcs::read (Vmcs::GUEST_RSP);

    if (mtd.xfer (Mtd::RIP_LEN)) {
        eip      = Vmcs::read (Vmcs::GUEST_RIP);
        inst_len = Vmcs::read (Vmcs::EXI_INST_LEN);
    }

    if (mtd.xfer (Mtd::RFLAGS))
        efl = Vmcs::read (Vmcs::GUEST_RFLAGS);

    if (mtd.xfer (Mtd::DS_ES)) {
        ds_sel   = Vmcs::read (Vmcs::GUEST_SEL_DS);
        es_sel   = Vmcs::read (Vmcs::GUEST_SEL_ES);
        ds_base  = Vmcs::read (Vmcs::GUEST_BASE_DS);
        es_base  = Vmcs::read (Vmcs::GUEST_BASE_ES);
        ds_limit = Vmcs::read (Vmcs::GUEST_LIMIT_DS);
        es_limit = Vmcs::read (Vmcs::GUEST_LIMIT_ES);
        ds_ar    = Vmcs::read (Vmcs::GUEST_AR_DS);
        es_ar    = Vmcs::read (Vmcs::GUEST_AR_ES);
    }

    if (mtd.xfer (Mtd::FS_GS)) {
        fs_sel   = Vmcs::read (Vmcs::GUEST_SEL_FS);
        gs_sel   = Vmcs::read (Vmcs::GUEST_SEL_GS);
        fs_base  = Vmcs::read (Vmcs::GUEST_BASE_FS);
        gs_base  = Vmcs::read (Vmcs::GUEST_BASE_GS);
        fs_limit = Vmcs::read (Vmcs::GUEST_LIMIT_FS);
        gs_limit = Vmcs::read (Vmcs::GUEST_LIMIT_GS);
        fs_ar    = Vmcs::read (Vmcs::GUEST_AR_FS);
        gs_ar    = Vmcs::read (Vmcs::GUEST_AR_GS);
    }

    if (mtd.xfer (Mtd::CS_SS)) {
        cs_sel   = Vmcs::read (Vmcs::GUEST_SEL_CS);
        ss_sel   = Vmcs::read (Vmcs::GUEST_SEL_SS);
        cs_base  = Vmcs::read (Vmcs::GUEST_BASE_CS);
        ss_base  = Vmcs::read (Vmcs::GUEST_BASE_SS);
        cs_limit = Vmcs::read (Vmcs::GUEST_LIMIT_CS);
        ss_limit = Vmcs::read (Vmcs::GUEST_LIMIT_SS);
        cs_ar    = Vmcs::read (Vmcs::GUEST_AR_CS);
        ss_ar    = Vmcs::read (Vmcs::GUEST_AR_SS);
    }

    if (mtd.xfer (Mtd::TR)) {
        tr_sel   = Vmcs::read (Vmcs::GUEST_SEL_TR);
        tr_base  = Vmcs::read (Vmcs::GUEST_BASE_TR);
        tr_limit = Vmcs::read (Vmcs::GUEST_LIMIT_TR);
        tr_ar    = Vmcs::read (Vmcs::GUEST_AR_TR);
    }

    if (mtd.xfer (Mtd::LDTR)) {
        ld_sel   = Vmcs::read (Vmcs::GUEST_SEL_LDTR);
        ld_base  = Vmcs::read (Vmcs::GUEST_BASE_LDTR);
        ld_limit = Vmcs::read (Vmcs::GUEST_LIMIT_LDTR);
        ld_ar    = Vmcs::read (Vmcs::GUEST_AR_LDTR);
    }

    if (mtd.xfer (Mtd::GDTR)) {
        gd_base  = Vmcs::read (Vmcs::GUEST_BASE_GDTR);
        gd_limit = Vmcs::read (Vmcs::GUEST_LIMIT_GDTR);
    }

    if (mtd.xfer (Mtd::IDTR)) {
        id_base  = Vmcs::read (Vmcs::GUEST_BASE_IDTR);
        id_limit = Vmcs::read (Vmcs::GUEST_LIMIT_IDTR);
    }

    if (mtd.xfer (Mtd::CR)) {
        cr2 = regs->cr2;
        cr3 = regs->cr3_shadow;
        cr0 = regs->cr0_shadow | (Vmcs::read (Vmcs::GUEST_CR0) & ~Vmcs::cr0_fix0 & Vmcs::cr0_fix1);
        cr4 = regs->cr4_shadow | (Vmcs::read (Vmcs::GUEST_CR4) & ~Vmcs::cr4_fix0 & Vmcs::cr4_fix1);
    }

    if (mtd.xfer (Mtd::DR))
        dr7 = Vmcs::read (Vmcs::GUEST_DR7);

    if (mtd.xfer (Mtd::SYSENTER)) {
        sysenter_cs  = Vmcs::read (Vmcs::GUEST_SYSENTER_CS);
        sysenter_esp = Vmcs::read (Vmcs::GUEST_SYSENTER_ESP);
        sysenter_eip = Vmcs::read (Vmcs::GUEST_SYSENTER_EIP);
    }

    if (mtd.xfer (Mtd::INTR_INFO)) {
        intr_info  = Vmcs::read (Vmcs::EXI_INTR_INFO);
        intr_error = Vmcs::read (Vmcs::EXI_INTR_ERROR);
    }

    if (mtd.xfer (Mtd::INTR_ACTV)) {
        intr_state = Vmcs::read (Vmcs::GUEST_INTR_STATE);
        actv_state = Vmcs::read (Vmcs::GUEST_ACTV_STATE);
    }

    if (mtd.xfer (Mtd::QUAL))
        qual = Vmcs::read (Vmcs::EXI_QUALIFICATION);

    if (mtd.xfer (Mtd::TSC)) {
        tsc_lo = Vmcs::read (Vmcs::TSC_OFFSET);
        tsc_hi = Vmcs::read (Vmcs::TSC_OFFSET_HI);
    }

    if (mtd.xfer (Mtd::ADDR))
        addr = regs->ept_fault;
}

mword *Utcb::save (Exc_regs *regs, Vmcs *vmcs, Mtd mtd)
{
    if (mtd.xfer (Mtd::GPR_ACDB)) {
        regs->eax = eax;
        regs->ecx = ecx;
        regs->edx = edx;
        regs->ebx = ebx;
    }

    if (mtd.xfer (Mtd::GPR_BSD)) {
        regs->ebp = ebp;
        regs->esi = esi;
        regs->edi = edi;
    }

    vmcs->make_current();

    if (mtd.xfer (Mtd::RSP))
        Vmcs::write (Vmcs::GUEST_RSP, esp);
    if (mtd.xfer (Mtd::RIP_LEN))
        Vmcs::write (Vmcs::GUEST_RIP, eip);
    if (mtd.xfer (Mtd::RFLAGS))
        Vmcs::write (Vmcs::GUEST_RFLAGS, efl);

    if (mtd.xfer (Mtd::DS_ES)) {
        Vmcs::write (Vmcs::GUEST_SEL_DS,   ds_sel);
        Vmcs::write (Vmcs::GUEST_SEL_ES,   es_sel);
        Vmcs::write (Vmcs::GUEST_BASE_DS,  ds_base);
        Vmcs::write (Vmcs::GUEST_BASE_ES,  es_base);
        Vmcs::write (Vmcs::GUEST_LIMIT_DS, ds_limit);
        Vmcs::write (Vmcs::GUEST_LIMIT_ES, es_limit);
        Vmcs::write (Vmcs::GUEST_AR_DS,    ds_ar);
        Vmcs::write (Vmcs::GUEST_AR_ES,    es_ar);
    }

    if (mtd.xfer (Mtd::FS_GS)) {
        Vmcs::write (Vmcs::GUEST_SEL_FS,   fs_sel);
        Vmcs::write (Vmcs::GUEST_SEL_GS,   gs_sel);
        Vmcs::write (Vmcs::GUEST_BASE_FS,  fs_base);
        Vmcs::write (Vmcs::GUEST_BASE_GS,  gs_base);
        Vmcs::write (Vmcs::GUEST_LIMIT_FS, fs_limit);
        Vmcs::write (Vmcs::GUEST_LIMIT_GS, gs_limit);
        Vmcs::write (Vmcs::GUEST_AR_FS,    fs_ar);
        Vmcs::write (Vmcs::GUEST_AR_GS,    gs_ar);
    }

    if (mtd.xfer (Mtd::CS_SS)) {
        Vmcs::write (Vmcs::GUEST_SEL_CS,   cs_sel);
        Vmcs::write (Vmcs::GUEST_SEL_SS,   ss_sel);
        Vmcs::write (Vmcs::GUEST_BASE_CS,  cs_base);
        Vmcs::write (Vmcs::GUEST_BASE_SS,  ss_base);
        Vmcs::write (Vmcs::GUEST_LIMIT_CS, cs_limit);
        Vmcs::write (Vmcs::GUEST_LIMIT_SS, ss_limit);
        Vmcs::write (Vmcs::GUEST_AR_CS,    cs_ar);
        Vmcs::write (Vmcs::GUEST_AR_SS,    ss_ar);
    }

    if (mtd.xfer (Mtd::TR)) {
        Vmcs::write (Vmcs::GUEST_SEL_TR,     tr_sel);
        Vmcs::write (Vmcs::GUEST_BASE_TR,    tr_base);
        Vmcs::write (Vmcs::GUEST_LIMIT_TR,   tr_limit);
        Vmcs::write (Vmcs::GUEST_AR_TR,      tr_ar);
    }

    if (mtd.xfer (Mtd::LDTR)) {
        Vmcs::write (Vmcs::GUEST_SEL_LDTR,   ld_sel);
        Vmcs::write (Vmcs::GUEST_BASE_LDTR,  ld_base);
        Vmcs::write (Vmcs::GUEST_LIMIT_LDTR, ld_limit);
        Vmcs::write (Vmcs::GUEST_AR_LDTR,    ld_ar);
    }

    if (mtd.xfer (Mtd::GDTR)) {
        Vmcs::write (Vmcs::GUEST_BASE_GDTR,  gd_base);
        Vmcs::write (Vmcs::GUEST_LIMIT_GDTR, gd_limit);
    }

    if (mtd.xfer (Mtd::IDTR)) {
        Vmcs::write (Vmcs::GUEST_BASE_IDTR,  id_base);
        Vmcs::write (Vmcs::GUEST_LIMIT_IDTR, id_limit);
    }

    if (mtd.xfer (Mtd::CR)) {
        mword cr0_mask = Vmcs::cr0_fix0 | ~Vmcs::cr0_fix1;
        mword cr4_mask = Vmcs::cr4_fix0 | ~Vmcs::cr4_fix1;
        regs->cr2 = cr2;
        regs->cr3_shadow = cr3;
        regs->cr0_shadow = cr0 & cr0_mask;
        regs->cr4_shadow = cr4 & cr4_mask;
        Vmcs::write (Vmcs::CR0_READ_SHADOW, regs->cr0_shadow);
        Vmcs::write (Vmcs::CR4_READ_SHADOW, regs->cr4_shadow);
        Vmcs::write (Vmcs::GUEST_CR0, (cr0 & ~cr0_mask) | Vmcs::cr0_fix0);
        Vmcs::write (Vmcs::GUEST_CR4, (cr4 & ~cr4_mask) | Vmcs::cr4_fix0);
    }

    if (mtd.xfer (Mtd::DR))
        Vmcs::write (Vmcs::GUEST_DR7, dr7);

    if (mtd.xfer (Mtd::SYSENTER)) {
        Vmcs::write (Vmcs::GUEST_SYSENTER_CS,  sysenter_cs);
        Vmcs::write (Vmcs::GUEST_SYSENTER_ESP, sysenter_esp);
        Vmcs::write (Vmcs::GUEST_SYSENTER_EIP, sysenter_eip);
    }

    if (mtd.xfer (Mtd::INTR_INFO)) {
        Vmcs::write (Vmcs::ENT_INTR_INFO,  intr_info);
        Vmcs::write (Vmcs::ENT_INTR_ERROR, intr_error);
    }

    if (mtd.xfer (Mtd::INTR_ACTV)) {
        Vmcs::write (Vmcs::GUEST_INTR_STATE, intr_state);
        Vmcs::write (Vmcs::GUEST_ACTV_STATE, actv_state);
    }

    if (mtd.xfer (Mtd::CTRL))
        Vmcs::write (Vmcs::CPU_EXEC_CTRL_1, (ctrl | static_cast<uint32>(Vmcs::ctrl_cpu)) & static_cast<uint32>(Vmcs::ctrl_cpu >> 32));

    if (mtd.xfer (Mtd::TSC)) {
        Vmcs::write (Vmcs::TSC_OFFSET,    tsc_lo);
        Vmcs::write (Vmcs::TSC_OFFSET_HI, tsc_hi);
    }

    if (mtd.xfer (Mtd::VTLB_FLUSH)) {

        if (EXPECT_FALSE (Cpu::spinner))
            screen.put (Cpu::spinner, 4, Console_vga::COLOR_LIGHT_RED, (++Cpu::vtlb_flush_count & 0xf)["0123456789ABCDEF"]);

        regs->vtlb->flush (mtd.xfer (Mtd::VTLB_FLUSH_ALL));
    }

    return item;
}
