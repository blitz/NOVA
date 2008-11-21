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

#pragma once

#include "compiler.h"
#include "mdb.h"
#include "mtd.h"
#include "refptr.h"
#include "spinlock.h"
#include "slab.h"
#include "types.h"

class Wq;

class Pt
{
    private:
        static Slab_cache cache;

    public:
        Refptr<Wq>  wq;
        Mtd         mtd;
        mword       id;
        mword       ip;

        Spinlock    lock;
        Mn          node;

        // Allocator
        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return cache.alloc();
        }

        // Deallocator
        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            cache.free (ptr);
        }

        // Constructor
        Pt (Wq *, Mtd, mword, mword);

        ~Pt();
};
