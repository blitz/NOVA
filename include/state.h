/*
 * CPU Exception State
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

class Vtlb;

class Sys_regs
{
    public:
        mword   edi;                // 0x0
        mword   esi;                // 0x4
        mword   ebp;                // 0x8
        mword   cr2;                // 0xc
        mword   ebx;                // 0x10
        mword   edx;                // 0x14
        mword   ecx;                // 0x18
        mword   eax;                // 0x1c

        enum Status
        {
            SUCCESS,
            TIMEOUT,
            BAD_SYS,
            BAD_CAP,
            BAD_MEM
        };

        enum
        {
            MSG_CALL,
            MSG_REPLY,
            CREATE_PT,
            CREATE_WQ,
            CREATE_EC,
            CREATE_SC,
            CREATE_PD,
            REVOKE,
        };

        ALWAYS_INLINE
        inline void set_status (Status status) { eax = status; }
};

class Exc_regs : public Sys_regs
{
    friend class Utcb;

    public:
        union {
            struct {
                mword   gs;         // 0x20
                mword   fs;         // 0x24
                mword   es;         // 0x28
                mword   ds;         // 0x2c
                mword   err;        // 0x30
                mword   vec;        // 0x34
                mword   eip;        // 0x38
                mword   cs;         // 0x3c
                mword   efl;        // 0x40
                mword   esp;        // 0x44
                mword   ss;         // 0x48
            };
            struct {
                uint64  tsc_vmexit; // 0x20
                mword   cr0_shadow; // 0x28
                mword   cr3_shadow; // 0x2c
                mword   cr4_shadow; // 0x30
                mword   dst_portal; // 0x34
                mword   ept_fault;  // 0x38
                Vtlb *  vtlb;       // 0x3c
            };
        };

        ALWAYS_INLINE
        inline bool user() const { return cs & 3; }
};
