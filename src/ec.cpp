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
#include "elf.h"
#include "extern.h"
#include "gsi.h"
#include "hip.h"
#include "memory.h"
#include "pt.h"
#include "ptab.h"
#include "sc.h"
#include "selectors.h"
#include "stdio.h"
#include "utcb.h"
#include "vmx.h"
#include "vtlb.h"
#include "wq.h"

// EC Cache
Slab_cache Ec::cache ("EC", sizeof (Ec), 8);

// Current EC
Ec *Ec::current;

// XXX
uint32 Ec::debug_xxx()
{
  return Buddy::ptr_to_phys(Ec::current->pd->cpu_ptab());
}

// Constructors
Ec::Ec (Pd *p, void (*c)()) : continuation (c), utcb (0), pd (p), ref (0) {}

Ec::Ec (Pd *p, void (*c)(), mword addr) : continuation (c), pd (p), ref (1)
{
    if (addr) {
        regs.cs  = SEL_USER_CODE;
        regs.ds  = SEL_USER_DATA;
        regs.es  = SEL_USER_DATA;
        regs.ss  = SEL_USER_DATA;
        regs.efl = Cpu::EFL_IF;

        utcb = new Utcb;

        pd->Space_mem::insert (addr,
                               Buddy::ptr_to_phys (utcb),
                               PAGE_SIZE,
                               Ptab::Attribute (Ptab::ATTR_USER |
                                                Ptab::ATTR_WRITABLE));

        trace (TRACE_SYSCALL, "EC:%p created (PD:%p UTCB:%p)", this, p, utcb);

    } else {
        regs.vtlb = new Vtlb;

        vmcs = new Vmcs (Buddy::ptr_to_phys (regs.vtlb),
                         Buddy::ptr_to_phys (pd->cpu_ptab()));

        trace (TRACE_SYSCALL, "EC:%p created (PD:%p VMCS:%p VTLB:%p)", this, p, vmcs, regs.vtlb);
    }
}

void Ec::ret_user_sysexit()
{
    if (EXPECT_TRUE (!(current->regs.vec & 0x80000000)))
        asm volatile ("mov %0, %%esp;"
                      "popa;"
                      "sti;"
                      "sysexit"
                      : : "g" (&current->regs) : "memory");

    send_exc_msg();
}

void Ec::ret_user_iret()
{
    if (EXPECT_TRUE (!(current->regs.vec & 0x80000000)))
        asm volatile ("mov %0, %%esp;"
                      "popa;"
                      "pop %%gs;"
                      "pop %%fs;"
                      "pop %%es;"
                      "pop %%ds;"
                      "add $8, %%esp;"
                      "iret"
                      : : "g" (&current->regs) : "memory");

    send_exc_msg();
}

void Ec::idle()
{
    Cpu::halt();

    Sc::schedule();
}

void Ec::root_invoke()
{
#if 1
    // Map VGA
    Pd::current->Space_mem::insert (0xb8000, 0xb8000, 8 * PAGE_SIZE,
                                    Ptab::Attribute (Ptab::ATTR_NOEXEC      |
                                                     Ptab::ATTR_UNCACHEABLE |
                                                     Ptab::ATTR_USER        |
                                                     Ptab::ATTR_WRITABLE));
#endif

    // Create GSI portals
    for (unsigned i = 0; i < NUM_GSI; i++)
        Pd::current->Space_obj::insert (NUM_PRE + i, Capability (new Pt (Gsi::gsi_table[i].wq = new Wq, Mtd (0), i, reinterpret_cast<mword>(&sys_ipc_gsi))));

    // Map hypervisor information page
    Pd::current->Space_mem::insert (LINK_ADDR - PAGE_SIZE,
                                    reinterpret_cast<Paddr>(&FRAME_H),
                                    PAGE_SIZE,
                                    Ptab::ATTR_USER);

    // Map root task image
    mword addr = reinterpret_cast<mword>(Ptab::current()->remap (Hip::root_addr));

    Elf_header *eh = reinterpret_cast<Elf_header *>(addr);
    if (!Hip::root_addr || *eh->ident != Elf_header::EH_MAGIC)
        current->kill (&current->regs, "No ELF");

    current->regs.eip = eh->entry;
    current->regs.esp = LINK_ADDR - PAGE_SIZE;

    Program_header *ph = reinterpret_cast<Program_header *>(addr + eh->ph_offset);

    for (unsigned i = 0; i < eh->ph_count; i++, ph++) {

        if (ph->type != Program_header::PT_LOAD)
            continue;

        Ptab::Attribute attr = Ptab::ATTR_USER;
        if (ph->flags & Program_header::PF_W)
            attr = Ptab::Attribute (attr | Ptab::ATTR_WRITABLE);
        if (!(ph->flags & Program_header::PF_X))
            attr = Ptab::Attribute (attr | Ptab::ATTR_NOEXEC);

        // XXX: Current limitation: fsize == msize so that roottask can be
        //      mapped without having to allocate BSS pages
        assert (ph->f_size == ph->m_size);
        assert (ph->v_addr % PAGE_SIZE == ph->f_offs % PAGE_SIZE);

        Pd::current->Space_mem::insert (align_dn (ph->v_addr, PAGE_SIZE),
                                        align_dn (Hip::root_addr + ph->f_offs, PAGE_SIZE),
                                        align_up (ph->f_size, PAGE_SIZE),
                                        attr);
    }

    ret_user_iret();
}

void Ec::preemption_handler()
{
    current->continuation = ret_user_iret;
    Sc::schedule();
}

void Ec::task_gate_handler()
{
    panic ("CPU%u: #TG at CS:%#x EIP:%#lx SS:%#x ESP:%#lx DS:%#x ES:%#x\n",
           Cpu::id,
           Tss::run.cs,
           Tss::run.eip,
           Tss::run.ss,
           Tss::run.esp,
           Tss::run.ds,
           Tss::run.es);
}

void Ec::exc_handler (Exc_regs *r)
{
    if (r->vec == 0xe) {
        pf_handler (r);
        return;
    }

    if (!r->user())
        current->kill (r, "EXC");

    send_exc_msg();
}

void Ec::pf_handler (Exc_regs *r)
{
    mword addr = r->cr2;

    // User fault
    if (r->err & Ptab::ERROR_USER)
        send_exc_msg();

    // Kern fault

    // #PF in MEM space
    if (addr >= LINK_ADDR && addr < LOCAL_SADDR) {
        Space_mem::page_fault (r);
        return;
    }

    // #PF in I/O space (including delimiter byte)
    if (addr >= IOBMP_SADDR && addr <= IOBMP_EADDR) {
        Space_io::page_fault (r);
        return;
    }

    // #PF in OBJ space
    if (addr >= OBJSP_SADDR) {
        Space_obj::page_fault (r);
        return;
    }

    current->kill (r, "#PF (kernel)");
}

void Ec::kill (Exc_regs *r, char const *reason)
{
    trace (0, "EC:%p CS:%#lx EIP:%#lx ADDR:%#lx ERR:%#lx KILLED (%s)",
           this, r->cs, r->eip, r->cr2, r->err, reason);

    Sc::schedule (true);
}
