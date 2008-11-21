/*
 * Portal
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

#include "pt.h"
#include "wq.h"

// Portal Cache
Slab_cache Pt::cache ("PT", sizeof (Pt), 16);

Pt::Pt (Wq *w, Mtd m, mword i, mword addr) : wq (w), mtd (m), id (i), ip (addr), node (Pd::current, id, 0)
{
    trace (TRACE_SYSCALL, "PT:%p created (WQ:%p ID:%#lx IP:%#lx)", this, w, i, addr);
}

Pt::~Pt()
{}
