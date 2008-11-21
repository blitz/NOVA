/*
 * Scheduling Context
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
#include "sc.h"

// SC Cache
Slab_cache Sc::cache ("SC", sizeof (Sc), 8);

// Current SC
Sc *Sc::current;

// Ready List
Sc *Sc::list[Sc::priorities];

unsigned Sc::prio_top;

Sc::Sc (Ec *e, unsigned p, unsigned q) : user (e), prio (p), full (q), left (0), ec (e)
{
    trace (TRACE_SYSCALL, "SC:%p created (EC:%p P:%u Q:%u)", this, e, p, q);

    user->set_sc (this);
}

Sc::~Sc()
{
    user->set_sc (0);
}

void Sc::make_current()
{
    ready_dequeue();

    current = this;

    user->make_current();
}

void Sc::schedule (bool suspend)
{
    assert (current);
    assert (!current->next);

    if (EXPECT_TRUE (!suspend))
        current->ready_enqueue();

    Sc *sc = list[prio_top];

    trace (TRACE_SCHEDULE, "--> %p (%02u)", sc, sc->left);

    sc->make_current();
}

void Sc::timer_tick()
{
    assert (!current->next);

    if (current->left)
        current->left--;
}
