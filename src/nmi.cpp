/*
 * Copyright (C) 2009, Julian Stecklina
 *
 *   ((
 *    ))     This file is COFFEEWARE. As long as you retain this notice
 *  |   |o)  you can do whatever you want with this code. If you think,
 *  |___|jgs it's worth it, you may buy the author a coffee in return.
 *
 */

#include "ec.h"
#include "cpu.h"
#include "nmi.h"
#include "gdt.h"
#include "string.h"

#ifdef NIXON

struct nmi_command_space nmi_command_space __attribute__ ((aligned (4096)));

// Calculates the checksum of the nmi_command_space. Should be zero.
static uint32 nmi_checksum()
{
    uint32 sum = 0;

    for (unsigned i = 0; i < (sizeof(nmi_command_space.word) / sizeof(uint32)); i++) {
        sum += nmi_command_space.word[i];
    }

    return sum;
}

void nmi_setup()
{
    memset(&nmi_command_space, 0, sizeof(struct nmi_command_space));
    nmi_command_space.checksum = nmi_checksum() - NIXON_MAGIC_VALUE;
    
    // Make sure the magic value becomes visible last.
    Cpu::store_fence();
    nmi_command_space.magic = NIXON_MAGIC_VALUE;
}

static void
save_debug_registers(void)
{
    nmi_command_space.dr0 = Cpu::get_dr0();
    nmi_command_space.dr1 = Cpu::get_dr1();
    nmi_command_space.dr2 = Cpu::get_dr2();
    nmi_command_space.dr3 = Cpu::get_dr3();
    nmi_command_space.dr6 = Cpu::get_dr6();
    nmi_command_space.dr7 = Cpu::get_dr7();
}

static void
load_debug_registers(void)
{
    // Disable all hardware breakpoints before attempting to change
    // them. (Just to shut up qemu.)
    Cpu::set_dr7(Cpu::DR7_EMPTY);
    Cpu::set_dr0(nmi_command_space.dr0);
    Cpu::set_dr1(nmi_command_space.dr1);
    Cpu::set_dr2(nmi_command_space.dr2);
    Cpu::set_dr3(nmi_command_space.dr3);
    Cpu::set_dr6(nmi_command_space.dr6);
    Cpu::set_dr7(nmi_command_space.dr7);
}

static inline bool
nmi_command_test(uint32 flag)
{
    return ((nmi_command_space.command & flag) != 0);
}

void Ec::nmi_handler()
{
    while (1) {
        struct nmi_command_space *cs = &nmi_command_space;
        bool verbose = nmi_command_test(NIXON_VERBOSE);

        if (verbose) {
            printf ("NMI at CS:%#x EIP:%#lx SS:%#x ESP:%#lx\n",
                    Tss::run.cs,
                    Tss::run.eip,
                    Tss::run.ss,
                    Tss::run.esp);
        }

        // Task switching does not save CR3 (and some other stuff). Try to
        // restore that from kernel data structures.
        uint32 cur_ptab = Buddy::ptr_to_phys(current->pd->cpu_ptab());
        Tss::run.cr3 = cur_ptab;

        // Verify checksum.
        assert(nmi_checksum() == 0);

        // Save paging configuration, so we can do page table lookups from
        // the host.
        cs->cr3 = Tss::run.cr3;
        cs->cr4 = Cpu::get_cr4();
        cs->tss_location = reinterpret_cast<uint32>(&Tss::run);

        // Set status flags.
        cs->status = 0;
        switch (Tss::nmi.link) {
        case SEL_TSS_RUN: cs->status |= NIXON_RUN; break;
        case SEL_TSS_DBG: cs->status |= NIXON_DBG; break;
        default: cs->status |= NIXON_PANIC | NIXON_DBG; break;
        }

        // Reset the debug exception task.
        extern char dbg_handler;
        Gdt::gdt[SEL_TSS_DBG >> 3].unbusy_tss();
        Tss::dbg.eip = reinterpret_cast<uint32>(&dbg_handler);
        Tss::dbg.link = SEL_TSS_RUN;

        // We cannot currently swap the real debug registers with our shadow copy.
        assert(!(nmi_command_test(NIXON_DR_TO_CS) & nmi_command_test(NIXON_DR_FROM_CS)));

        if (nmi_command_test(NIXON_DR_TO_CS)) {
            save_debug_registers();
        }

        if (nmi_command_test(NIXON_DR_FROM_CS)) {
            load_debug_registers();
        }

        // TSS housekeeping
        uint16 target_sel = static_cast<uint16>(nmi_command_test(NIXON_STOP) ? SEL_TSS_DBG : SEL_TSS_RUN);
        Tss::nmi.link = target_sel;
        Gdt::gdt[target_sel >> 3].busy_tss();

        // Update command space checksum. The counter is incremented
        // after that, so that Nixon can poll on it.
        cs->command  = 0;
        cs->checksum = 0;
        cs->checksum = ~nmi_checksum();
        // Make sure the counter becomes visible last.
        Cpu::store_fence();
        cs->counter++;

        // Verify checksum.
        assert(nmi_checksum() == 0);
        assert(nmi_command_space.inside_dbg == 0);

        // We need to return via iret to tell the processor's NMI handling
        // circuits that we handled the NMI. The problem is that iret
        // stores a new EIP in our TSS which points after the iret
        // instruction. That is why we use a loop here.

        asm volatile ("iret" ::: "memory" );

        // Our stack is unchanged when we enter here.
    }

    // Not reached
}

#endif  // NIXON

// EOF
