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

#pragma once

#include "compiler.h"
#include "refptr.h"
#include "slab.h"
#include "stdio.h"
#include "types.h"

class Ec;

class Sc
{
    private:
        Ec *        user;       // this should be first (offset 0)
        Sc *        prev;
        Sc *        next;
        unsigned    prio;
        unsigned    full;
        unsigned    left;
        Refptr<Ec>  ec;

        static unsigned const priorities = 256;

        // SC Cache
        static Slab_cache cache;

        // Ready List
        static Sc *list[priorities] CPULOCAL;

        static unsigned prio_top CPULOCAL;

        ALWAYS_INLINE NORETURN
        inline void make_current();

    public:
        // Current SC
        static Sc *current CPULOCAL_HOT;

        static unsigned const default_prio = 1;
        static unsigned const default_quantum = 10;

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
        Sc (Ec *, unsigned, unsigned);

        ~Sc();

        ALWAYS_INLINE
        inline void set_user (Ec *e)
        {
            user = e;
        }

        ALWAYS_INLINE
        inline void ready_enqueue()
        {
            if (prio > prio_top)
                prio_top = prio;

            if (!list[prio])
                list[prio] = prev = next = this;
            else {
                next = list[prio];
                prev = list[prio]->prev;
                next->prev = prev->next = this;
                if (left)
                    list[prio] = this;
            }

            trace (TRACE_SCHEDULE, "ENQ:%p (%02u) PRIO:%u (%u)", this, left, prio, prio_top);
        
            if (!left)
                left = full;
        }

        ALWAYS_INLINE
        inline void ready_dequeue()
        {
            if (!next)
                return;
    
            if (list[prio] == this)
                list[prio] = next == this ? 0 : next;
    
            next->prev = prev;
            prev->next = next;
            next = 0;

            while (!list[prio_top] && prio_top)
                prio_top--;

            trace (TRACE_SCHEDULE, "DEQ:%p (%02u) PRIO:%u (%u)", this, left, prio, prio_top);
        }

        NORETURN
        static void schedule (bool suspend = false);

        static void timer_tick();
};
