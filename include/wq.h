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

#pragma once

#include "atomic.h"
#include "compiler.h"
#include "ec.h"
#include "slab.h"
#include "types.h"

class Wq
{
    private:
        Ec *                wait_queue;
        Ec *                send_queue;
        unsigned            ref;

        static Slab_cache   cache;

    public:
        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }

        Wq();

        ALWAYS_INLINE
        inline void add_ref() { Atomic::add (ref, 1); }

        ALWAYS_INLINE
        inline bool del_ref() { return Atomic::sub (ref, 1); }

        ALWAYS_INLINE
        inline void wait_enqueue (Ec *ec)
        {
            ec->next = wait_queue;
            wait_queue = ec;
        }

        ALWAYS_INLINE
        inline Ec *wait_dequeue()
        {
            Ec *ec = wait_queue;

            if (EXPECT_TRUE (ec))
                wait_queue = ec->next;

            return ec;
        }

        ALWAYS_INLINE
        inline void send_enqueue (Ec *ec)
        {
            ec->next = send_queue;
            send_queue = ec;
        }

        ALWAYS_INLINE
        inline Ec *send_dequeue()
        {
            Ec *ec = send_queue;

            if (EXPECT_TRUE (ec))
                send_queue = ec->next;

            return ec;
        }

        ALWAYS_INLINE NORETURN
        inline void block (void (*cont)())
        {
            send_enqueue (Ec::current);
            Ec::current->continuation = cont;
            Sc::schedule (true);         
        }

};
