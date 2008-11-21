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

#pragma once

#include "compiler.h"
#include "slab.h"
#include "types.h"

class Pd;

class Mn
{
    private:
        static Slab_cache cache;

        Mn *        prev;
        Mn *        next;
        unsigned    depth;

    public:
        Pd *        pd;
        mword       cd;
        mword       attr;

        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return cache.alloc();
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            cache.free (ptr);
        }

        Mn (Pd *, mword, mword, unsigned = 0);

        ALWAYS_INLINE
        inline bool root() const
        {
            return !depth;
        }

        Mn * lookup (Pd *, mword);
        Mn * remove_child();
        void create_child (Pd *, mword, mword);
};

class Mr
{
    public:
        Spinlock    lock;
        Mn *        node;
};
