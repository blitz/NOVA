/*
 * Wait Queue
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

#include "wq.h"

// WQ Cache
Slab_cache Wq::cache ("WQ", sizeof (Wq), 8);

// Constructor
Wq::Wq() : wait_queue (0), send_queue (0), ref (1)
{
    trace (TRACE_SYSCALL, "WQ:%p created", this);
}
