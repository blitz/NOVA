/*
 * Execution Context
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
#include "capability.h"
#include "compiler.h"
#include "pd.h"
#include "refptr.h"
#include "sc.h"
#include "slab.h"
#include "state.h"
#include "syscall.h"
#include "tss.h"
#include "types.h"

class Utcb;
class Vmcs;

class Ec
{
    friend class Wq;

    private:
        Exc_regs regs;                              // 0x0, must be first
        void (*continuation)();                     // 0x4c
        Reply_capability rp;                        // 0x50, 0x54
        Ec *next;                                   // 0x58

        union {                                     // 0x5c
            Utcb *utcb;
            Vmcs *vmcs;
        };

        Refptr<Pd>  pd;                             // 0x60
        Sc *        sc;                             // 0x64
        unsigned    ref;

        static Slab_cache cache;

        REGPARM (1)
        static void exc_handler (Exc_regs *) asm ("exc_handler");

        NORETURN
        static void vmx_handler() asm ("vmx_handler");

        NORETURN
        static inline void vmx_exception();

        NORETURN
        static inline void vmx_extint();

    public:
        static Ec *current CPULOCAL_HOT;

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }

        Ec (Pd *, void (*)());
        Ec (Pd *, void (*)(), mword);

        ALWAYS_INLINE
        inline void add_ref() { Atomic::add (ref, 1); }

        ALWAYS_INLINE
        inline bool del_ref() { return Atomic::sub (ref, 1); }

        ALWAYS_INLINE
        inline void set_sc (Sc *s)
        {
            assert (!sc);
            sc = s;
        }

        ALWAYS_INLINE
        inline void release()
        {
            assert (sc);
            sc->ready_enqueue();
        }

        ALWAYS_INLINE NORETURN
        inline void make_current()
        {
            current = this;

            Tss::run.esp0 = reinterpret_cast<mword>(&regs + 1);

            pd->make_current();

            asm volatile ("mov %0, %%esp;"
                          "jmp *%1"
                          : : "g" (KSTCK_EADDR), "g" (continuation) : "memory"); UNREACHED;
        }

        NORETURN
        static void ret_user_vmresume();

        NORETURN
        static void ret_user_iret();

        NORETURN
        static void ret_user_sysexit();

        ALWAYS_INLINE NORETURN
        static inline void sys_finish (Sys_regs *param, Sys_regs::Status status);

        NORETURN
        static void send_exc_msg();

        NORETURN
        static void send_vmx_msg();

        ALWAYS_INLINE NORETURN
        inline void recv_ipc_msg (mword, unsigned = 0);

        NORETURN
        static void sys_ipc_gsi (Pt *);

        HOT NORETURN
        static void sys_ipc_call();

        HOT NORETURN
        static void sys_ipc_reply();

        NORETURN
        static void sys_create_pt();

        NORETURN
        static void sys_create_wq();

        NORETURN
        static void sys_create_ec();

        NORETURN
        static void sys_create_sc();

        NORETURN
        static void sys_create_pd();

        NORETURN
        static void sys_revoke();

        NORETURN
        static void idle();

        NORETURN
        static void root_invoke();

        HOT NORETURN REGPARM (1)
        static void syscall_handler (uint8) asm ("syscall_handler");

        NORETURN
        static void preemption_handler() asm ("preemption_handler");

        NORETURN
        static void task_gate_handler() asm ("task_gate_handler");

        static void pf_handler (Exc_regs *);

        NORETURN
        void kill (Exc_regs *, char const *);
};
