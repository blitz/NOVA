/*
 * User Thread Control Block (UTCB)
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

#include "buddy.h"
#include "compiler.h"
#include "crd.h"
#include "mtd.h"
#include "types.h"

class Exc_regs;
class Vmcs;

class Utcb
{
    public:
        mword   pid;
        Mtd     mtr;
        Crd     crd;
        mword   tls;

    private:
        union {
            struct {
                mword   eax;
                mword   ecx;
                mword   edx;
                mword   ebx;
                mword   esp;
                mword   ebp;
                mword   esi;
                mword   edi;
                mword   eip;
                mword   efl;
                mword   dr7;
                mword   addr;
                mword   cr0, cr2, cr3, cr4;
                mword   es_sel, es_base, es_limit, es_ar;
                mword   cs_sel, cs_base, cs_limit, cs_ar;
                mword   ss_sel, ss_base, ss_limit, ss_ar;
                mword   ds_sel, ds_base, ds_limit, ds_ar;
                mword   fs_sel, fs_base, fs_limit, fs_ar;
                mword   gs_sel, gs_base, gs_limit, gs_ar;
                mword   tr_sel, tr_base, tr_limit, tr_ar;
                mword   ld_sel, ld_base, ld_limit, ld_ar;
                mword           gd_base, gd_limit;
                mword           id_base, id_limit;
                uint32  tsc_lo;
                uint32  tsc_hi;
                mword   sysenter_cs, sysenter_esp, sysenter_eip;
                mword   intr_info, intr_error;
                mword   intr_state, actv_state;
                mword   inst_len;
                mword   qual;
                mword   ctrl;
            };
            mword mr[];
        };
        mword item[];

    public:

        ALWAYS_INLINE
        static inline void *operator new (size_t)
        {
            return Buddy::allocator.alloc (0, Buddy::FILL_0);
        }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr)
        {
            Buddy::allocator.free (reinterpret_cast<mword>(ptr));
        }

        ALWAYS_INLINE NONNULL
        inline mword *save (Utcb *dst, unsigned untyped)
        {
            dst->mtr = Mtd (untyped);
            mword *d = dst->mr, *s = mr;
#if 0
            asm volatile ("rep; movsl" : "+D" (d), "+S" (s), "+c" (mtd) : : "memory");
#else
            for (unsigned i = 0; i < untyped; i++)
                *d++ = *s++;    
#endif

            return s;
        }

        void    load (Exc_regs *, Mtd);
        mword * save (Exc_regs *, Mtd);

        void    load (Exc_regs *, Vmcs *, Mtd);
        mword * save (Exc_regs *, Vmcs *, Mtd);
};
