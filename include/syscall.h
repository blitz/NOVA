/*
 * System-Call Interface
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
#include "crd.h"
#include "mtd.h"
#include "state.h"
#include "types.h"

class Sys_ipc_recv : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline void set_sp (mword sp) { ecx = sp; }

        ALWAYS_INLINE
        inline void set_ip (mword ip) { edx = ip; }
};

class Sys_ipc_send : public Exc_regs
{
    public:
        enum
        {
            DISABLE_BLOCKING    = 1ul << 0,
            DISABLE_DONATION    = 1ul << 1,
            DISABLE_REPLYCAP    = 1ul << 2
        };

        ALWAYS_INLINE
        inline unsigned pt() const { return edi; }

        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (esi); }

        ALWAYS_INLINE
        inline unsigned flags() const { return eax >> 8 & 0xff; }
};

class Sys_ipc_repl : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned wq() const { return edi; }

        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (esi); }
};

class Sys_create_pd : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned pd() const { return edi; }

        ALWAYS_INLINE
        inline Crd crd() const { return Crd (esi); }

        ALWAYS_INLINE
        inline mword utcb() const { return ebx; }
};

class Sys_create_ec : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned ec() const { return edi; }

        ALWAYS_INLINE
        inline unsigned wq() const { return esi; }

        ALWAYS_INLINE
        inline mword utcb() const { return ebx; }

        ALWAYS_INLINE
        inline mword esp() const { return ebp; }
};

class Sys_create_sc : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned sc() const { return edi; }

        ALWAYS_INLINE
        inline unsigned ec() const { return esi; }

        ALWAYS_INLINE
        inline mword prio() const { return ebx; }

        ALWAYS_INLINE
        inline mword quantum() const { return ebp; }
};

class Sys_create_wq : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned wq() const { return edi; }
};

class Sys_create_pt : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline unsigned pt() const { return edi; }

        ALWAYS_INLINE
        inline unsigned wq() const { return esi; }

        ALWAYS_INLINE
        inline Mtd mtd() const { return Mtd (ebx); }

        ALWAYS_INLINE
        inline mword eip() const { return ebp; }
};

class Sys_revoke : public Exc_regs
{
    public:
        ALWAYS_INLINE
        inline Crd crd() const { return Crd (edi); }

        ALWAYS_INLINE
        inline bool self() const { return esi; }
};
