/*
 * Capability
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
#include "types.h"

class Ec;
class Pd;
class Pt;
class Sc;
class Wq;

class Capability
{
    private:
        mword val;

    public:
        enum Type
        {
            NUL,
            PT,
            WQ,
            EC,
            SC,
            PD
        };

        Capability() : val (0) {}

        Capability (Pt *v) : val (reinterpret_cast<mword>(v) | PT) {}
        Capability (Wq *v) : val (reinterpret_cast<mword>(v) | WQ) {}
        Capability (Ec *v) : val (reinterpret_cast<mword>(v) | EC) {}
        Capability (Sc *v) : val (reinterpret_cast<mword>(v) | SC) {}
        Capability (Pd *v) : val (reinterpret_cast<mword>(v) | PD) {}

        ALWAYS_INLINE
        inline Type type() const
        {
            return static_cast<Type>(val & 7ul);
        }

        template <typename T>
        ALWAYS_INLINE
        inline T *obj() const
        {
            return reinterpret_cast<T *>(val & ~7ul);
        }
};

class Reply_capability
{
    private:
        Ec *ec;
        Sc *sc;
    
    public:
        Reply_capability() : ec (0), sc (0) {}

        Reply_capability (Ec *e, Sc *s) : ec (e), sc (s) {}

        ALWAYS_INLINE
        inline Ec *check_ec() const { return ec; }

        ALWAYS_INLINE
        inline Sc *check_sc() const { return sc; }
};
