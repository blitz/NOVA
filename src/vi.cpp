/*
 * Semaphore
 *
 * Copyright (C) 2012 Julian Stecklina <jsteckli@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
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

#include "vi.hpp"
#include "stdio.hpp"

INIT_PRIORITY (PRIO_SLAB)
Slab_cache Vi::cache (sizeof (Vi), 32);

Vi::Vi (Pd *own, mword sel, Ec *e, mword evt)
  : Kobject (VI, static_cast<Space_obj *>(own), sel), ec (e), event (evt)
{
    trace (TRACE_SYSCALL, "VI:%p created (EC:%p EVT:%lu)", this, e, event);
}

// EOF
