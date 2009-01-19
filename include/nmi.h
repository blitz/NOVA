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
#define NIXON_MAGIC_VALUE 0xC6482010

struct nmi_command_space {
  // We fill in this magic value, when we are ready to be debugged.
  volatile uint32 magic;

  // Used to synchronize with debugging host. This is increased by the
  // NMI handler, when it has produced a consistent state.
  volatile uint32 counter;

  // Set to non-0 to enable diagnostic output.
  volatile uint32 verbose;

  // Set to non-0 to block inside the NMI handler. Must be set to 0
  // from the debugging host for the NMI handler to return.
  volatile uint32 stop;

  volatile uint32 cr3;          // Kernel page table and CR4. Set on
  volatile uint32 cr4;          // first NMI.


  // If the NMI handler detects that a debug exception has occured,
  // this is set. This is already set in dbg_handler, so Nixon may
  // poll this memory address instead of sending NMIs to check.
  // XXX This is referenced in dbg.S by offset.
  volatile uint32 inside_dbg;

  // Copies of the debug registers.
  volatile uint32 dr[8];
};

extern struct nmi_command_space nmi_command_space;

#endif

// EOF
