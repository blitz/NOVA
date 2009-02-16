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
    struct {
      // We fill in this magic value, when we are ready to be debugged.
      uint32 magic;

      /// Fields that are written only by the stub.

      // If the NMI handler detects that a debug exception has occured
      // this is set to 1. The monitor has to poll this value and send
      // an interrupt when this happens.  XXX This is referenced in
      // dbg.S by offset. So this must be the second field!
      uint32 inside_dbg;

      // A counter that the monitor can poll on.
      uint32 counter;

      // A bit field to report status back to the monitor.
      uint32 status;

      // Current page table, CR4 and TSS location.
      uint32 cr0;
      uint32 cr3;
      uint32 cr4;
      int32  tss_offset; // Offset relative to start of CS


      /// Static fields. These are set by the stub and should not be changed.

      // Some kind of descriptive text to identify what kind of OS
      // this command space controls.
      char   description[16*4];

      // A window of physical memory (sizes are in bytes). Set by the
      // stub. The monitor interprets all memory references (except
      // the TSS location) as relative to this window. If both values
      // are 0, don't offset memory addresses.
      int32  phys_offset; // Offset relative to start of CS
      uint32 phys_size;

      // The type of MSI sent by the monitor. See section 9.11.2
      // "Message Data register format" for the complete
      // explanation. The special value 0 means that the monitor
      // should use NMIs. Using other values is untested.
      uint32 int_vec;

      /// Fields that are written only by the monitor.

      // A bit field to tell the NMI handler what to do.
      uint32 command;

      /// The following fields are written by both the stub and the monitor.

      // Copies of the debug registers.
      uint32 dr0;
      uint32 dr1;
      uint32 dr2;
      uint32 dr3;
      uint32 dr6;
      uint32 dr7;

      // All fields added together in mod 2^32 arithmetic should give
      // 0. This can also be 1 in some circumstances. See dbg.S.
      uint32 checksum;
    };
    uint32 word[19 + 16];
  };
};

extern struct nmi_command_space nmi_command_space;

void nmi_setup();

#endif

// EOF
