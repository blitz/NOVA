/*
 * Mapping Database
 *
 * Copyright (C) 2008, Udo Steinberg <udo@hypervisor.org>
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

#include "mdb.h"

Slab_cache Mn::cache ("MN", sizeof (Mn), 16);

Mn::Mn (Pd *p, mword c, mword a, unsigned d) : prev (this), next (this), depth (d), pd (p), cd (c), attr (a)
{}

Mn *Mn::lookup (Pd *p, mword c)
{
    for (Mn *node = this; node; node = node->next)
        if (node->pd == p && node->cd == c)
            return node;

    return 0;
}

Mn *Mn::remove_child()
{
    Mn *node = next;

    if (node->depth > depth) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        return node;
    }

    return 0;
}

void Mn::create_child (Pd *p, mword c, mword a)
{
    Mn *node = new Mn (p, c, a, depth + 1);

    node->next  = next;
    node->prev  = this;
    next->prev  = node;
    next = node;
}
