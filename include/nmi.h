/* -*- Mode: C++ -*- */
/*
 * Copyright (C) 2009, Julian Stecklina
 *
 *   ((
 *    ))     This file is COFFEEWARE. As long as you retain this notice
 *  |   |o)  you can do whatever you want with this code. If you think,
 *  |___|jgs it's worth it, you may buy the author a coffee in return.
 *
 */

#pragma once

#ifdef NIXON

#include "types.h"

#define NIXON_WAIT_DEBUGGER_ATTACH asm volatile ( "int3" ::: "memory" )

enum {
  NIXON_MAGIC_VALUE = 0xC6482010,
};

enum nmi_command {
  NIXON_VERBOSE    = 1L << 0,
  NIXON_DR_TO_CS   = 1L << 1,
  NIXON_DR_FROM_CS = 1L << 2,
  NIXON_STOP       = 1L << 3,   // Return to debug task (if not set
                                // return to normal execution).
};

enum nmi_status {
  NIXON_RUN        = 1L << 0,   // We interrupted normal execution.
  NIXON_DBG        = 1L << 1,   // We interrupted a debug exception.
  NIXON_PANIC      = 1L << 2,   // The kernel panic'd.
};

struct nmi_command_space {
  union {
    volatile uint32 word[14];
    struct {
      // We fill in this magic value, when we are ready to be debugged.
      volatile uint32 magic;

      // If the NMI handler detects that a debug exception has occured,
      // this is set. This is already set in dbg_handler, so Nixon may
      // poll this memory address instead of sending NMIs to check.
      // XXX This is referenced in dbg.S by offset. So this must be
      // the second field!
      volatile uint32 inside_dbg;

      // A counter that Nixon can poll on.
      volatile uint32 counter;

      // A bit field to tell the NMI handler what to do.
      volatile uint32 command;

      // A bit field to report status back to Nixon.
      volatile uint32 status;

      // Kernel page table and CR4. Set on first NMI.
      volatile uint32 cr3;
      volatile uint32 cr4;

      // Copies of the debug registers.
      volatile uint32 dr0;
      volatile uint32 dr1;
      volatile uint32 dr2;
      volatile uint32 dr3;
      volatile uint32 dr6;
      volatile uint32 dr7;

      // All fields added together in mod 2^32 arithmetic should give
      // 0. This can also be 1 in some circumstances. See dbg.S.
      volatile uint32 checksum;
    };
  };
};

extern struct nmi_command_space nmi_command_space;

void nmi_setup();

#endif

// EOF
