/*
 * Virtual IRQ (VI)
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

#pragma once

#include "ec.hpp"

class Vi : public Kobject
{
    private:
        static Slab_cache cache;

    public:
        Refptr<Ec> const ec_handler;
        Refptr<Ec> const ec_recall;
        mword            event;

        Vi (Pd *, mword, Ec *, Ec *, mword);

        void trigger ()
        {
            ec_handler->set_events(event, ec_recall);
        }

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
