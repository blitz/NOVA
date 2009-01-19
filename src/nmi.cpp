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

#ifdef NIXON

struct nmi_command_space nmi_command_space;

static void dump_tss(Tss &tss)
{
  printf("TSS EAX:%0lx EBX:%0lx ECX:%0lx EDX:%0lx\n", tss.eax, tss.ebx, tss.ecx, tss.edx);
  printf("    ESP:%0lx EBP:%0lx ESI:%0lx EDI:%0lx\n", tss.esp, tss.ebp, tss.esi, tss.edi);
  printf("    ES: %x CS: %x SS: %x DS: %x FS: %x GS: %x\n",
         tss.es, tss.cs, tss.ss, tss.ds, tss.fs, tss.gs);
  printf("    EFLAGS:%0lx CR3:%0lx\n", tss.eflags, tss.cr3);
}

static void
save_debug_registers(void)
{
  volatile uint32 *dr = nmi_command_space.dr;
  dr[0] = Cpu::get_dr0();
  dr[1] = Cpu::get_dr1();
  dr[2] = Cpu::get_dr2();
  dr[3] = Cpu::get_dr3();
  // DR4 and DR5 are reserved.
  dr[6] = Cpu::get_dr6();
  dr[7] = Cpu::get_dr7();

}

static void
load_debug_registers(void)
{
  volatile uint32 *dr = nmi_command_space.dr;

  Cpu::set_dr7(Cpu::DR7_EMPTY); // Disable all hardware breakpoints

  Cpu::set_dr0(dr[0]);
  Cpu::set_dr1(dr[1]);
  Cpu::set_dr2(dr[2]);
  Cpu::set_dr3(dr[3]);
  Cpu::set_dr6(dr[6]);
  Cpu::set_dr7(dr[7]);
}

void Ec::nmi_handler()
{
  while (1) {

    if (nmi_command_space.verbose) {
      printf ("NMI at CS:%#x EIP:%#lx SS:%#x ESP:%#lx\n",
              Tss::run.cs,
              Tss::run.eip,
              Tss::run.ss,
              Tss::run.esp);
    }

    // Check what task we interrupted.
    switch (Tss::nmi.link) {
    case SEL_TSS_RUN:
      // Interrupted normal execution.
      nmi_command_space.inside_dbg = 0;
      break;
    case SEL_TSS_DBG:
      // Interrupted the #DB handler. A debug exception has occured.
      nmi_command_space.inside_dbg = 1;
      nmi_command_space.stop = 1;
      break;
    default:
      panic("NMI handler: Interrupted unknown task.");
    }

    // Store debug registers.
    save_debug_registers();

    // Task switching does not save CR3 (and some other stuff). Try to
    // restore that from kernel data structures.
    uint32 cur_ptab = Buddy::ptr_to_phys(current->pd->cpu_ptab());
    Tss::run.cr3 = cur_ptab;

    // Save paging configuration, so we can do page table lookups from
    // the host.
    nmi_command_space.cr3 = Tss::run.cr3;
    nmi_command_space.cr4 = Cpu::get_cr4();

    // Dump TSS state
    if (nmi_command_space.verbose) {
      dump_tss(Tss::run);
      printf("Sleeping...\n");
    }

    // Tss::run contains a fairly complete image of the system state.
    Cpu::flush_cache();
    nmi_command_space.counter++;

    // If we are told to stop, spin until we are allowed to
    // continue. The debugging host can now examine inside_dbg,
    // cr3/cr4 and Tss:run to decide what to do.
    while (nmi_command_space.stop)
      Cpu::pause();


    if (Tss::nmi.link == SEL_TSS_DBG) {
      // We interrupted the #BP handler. Unbusy its TSS and return to normal kernel code.
      extern char dbg_handler;
      Tss::dbg.eip = reinterpret_cast<uint32>(&dbg_handler);
      nmi_command_space.inside_dbg = 0;
      Gdt::gdt[SEL_TSS_DBG >> 3].unbusy_tss();

      assert(Tss::dbg.link == SEL_TSS_RUN);
      Tss::nmi.link = SEL_TSS_RUN;
    }

    // Dump TSS state
    if (nmi_command_space.verbose) {
      dump_tss(Tss::run);
    }


    // Restore debug registers which were potentially modified by the
    // debugging host.
    load_debug_registers();

    // We need to return via iret to tell the processor's NMI handling
    // circuits that we handled the NMI. The problem is that iret
    // stores a new EIP in our TSS which points after the iret
    // instruction. That is why we use a loop here.

    asm volatile ("iret" ::: "memory" );

    // Assume that our stack is unchanged when we enter here.
  }

  // Not reached
}

#endif  // NIXON

// EOF
