/*
 * Execution Context
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
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

#include "counter.hpp"
#include "fpu.hpp"
#include "mtd.hpp"
#include "pd.hpp"
#include "queue.hpp"
#include "regs.hpp"
#include "sc.hpp"
#include "tss.hpp"
#include "lapic.hpp"
#include "vectors.hpp"

class Utcb;

class Ec : public Kobject, public Refcount, public Queue<Sc>
{
    friend class Queue<Ec>;

    private:
        void        (*cont)() ALIGNED (16);
        Cpu_regs    regs;
        Ec *        rcap;
        Utcb *      utcb;
        Refptr<Pd>  pd;
        Ec *        partner;
        Ec *        prev;
        Ec *        next;
        Fpu *       fpu;
        union {
            struct {
                uint16  cpu;
                uint16  glb;
            };
            uint32  xcpu;
        };
        unsigned const evt;

        static Slab_cache cache;

        REGPARM (1)
        static void handle_exc (Exc_regs *) asm ("exc_handler");

        NORETURN
        static void handle_vmx() asm ("vmx_handler");

        NORETURN
        static void handle_svm() asm ("svm_handler");

        NORETURN
        static void handle_tss() asm ("tss_handler");

        static void handle_exc_nm();
        static bool handle_exc_ts (Exc_regs *);
        static bool handle_exc_gp (Exc_regs *);
        static bool handle_exc_pf (Exc_regs *);

        static inline uint8 ifetch (mword);

        NORETURN
        static inline void svm_exception (mword);

        NORETURN
        static inline void svm_cr();

        NORETURN
        static inline void svm_invlpg();

        NORETURN
        static inline void vmx_exception();

        NORETURN
        static inline void vmx_extint();

        NORETURN
        static inline void vmx_invlpg();

        NORETURN
        static inline void vmx_cr();

        static bool fixup (mword &);

        NOINLINE
        static void handle_hazard (mword, void (*)());

        ALWAYS_INLINE
        inline Sys_regs *sys_regs() { return &regs; }

        ALWAYS_INLINE
        inline Exc_regs *exc_regs() { return &regs; }

        ALWAYS_INLINE
        inline void set_partner (Ec *p)
        {
            partner = p;
            partner->rcap = this;
            Sc::ctr_link++;
        }

        ALWAYS_INLINE
        inline unsigned clr_partner()
        {
            assert (partner == current);
            partner->rcap = nullptr;
            partner = nullptr;
            return Sc::ctr_link--;
        }

        ALWAYS_INLINE
        inline void redirect_to_iret()
        {
            regs.REG(sp) = regs.ARG_SP;
            regs.REG(ip) = regs.ARG_IP;
        }

    public:
        static Ec *current CPULOCAL_HOT;
        static Ec *fpowner CPULOCAL;

        Ec (Pd *, void (*)(), unsigned);
        Ec (Pd *, mword, Pd *, void (*)(), unsigned, unsigned, mword, mword);

        ALWAYS_INLINE
        inline void add_tsc_offset (uint64 tsc)
        {
            regs.add_tsc_offset (tsc);
        }

        ALWAYS_INLINE
        inline bool blocked() const { return next || !cont; }

        ALWAYS_INLINE NORETURN
        inline void make_current()
        {
            current = this;

            Tss::run.sp0 = reinterpret_cast<mword>(exc_regs() + 1);

            pd->make_current();

            asm volatile ("mov %0," EXPAND (PREG(sp);) "jmp *%1" : : "g" (CPU_LOCAL_STCK + PAGE_SIZE), "rm" (cont) : "memory"); UNREACHED;
        }

        ALWAYS_INLINE
        static inline Ec *remote (unsigned c)
        {
            return *reinterpret_cast<volatile typeof current *>(reinterpret_cast<mword>(&current) - CPU_LOCAL_DATA + HV_GLOBAL_CPUS + c * PAGE_SIZE);
        }

        NOINLINE
        void help (void (*c)())
        {
            if (EXPECT_TRUE (cont != dead)) {

                Counter::print<1,16> (++Counter::helping, Console_vga::COLOR_LIGHT_WHITE, SPN_HLP);
                current->cont = c;

                if (EXPECT_TRUE (++Sc::ctr_loop < 100))
                    activate();

                die ("Livelock");
            }
        }

        NOINLINE
        void block_sc()
        {
            {   Lock_guard <Spinlock> guard (lock);

                if (!blocked())
                    return;

                enqueue (Sc::current);
            }

            Sc::schedule (true);
        }

        ALWAYS_INLINE
        inline void recall()
        {
            if (!(regs.hazard() & HZD_RECALL)) {

                regs.set_hazard (HZD_RECALL);

                if (Cpu::id != cpu && Ec::remote (cpu) == this)
                    Lapic::send_ipi (cpu, VEC_IPI_RKE);

                if (next == this) {
                    // TODO: Sends a second IPI. Ugly.
                    next = NULL;
                    release();
                }
            }
        }


        ALWAYS_INLINE
        inline void release()
        {
            Lock_guard <Spinlock> guard (lock);

            for (Sc *s; (s = dequeue()); s->remote_enqueue()) ;
        }

        ALWAYS_INLINE
        inline void wait_event()
        {
            // XXX Hacky because Ec considers itself blocked, iff it
            // is queued in a list. This can be beautified when
            // semaphores are gone.
            next = this;
            // XXX Is this racy? But we are doing almost the same
            // thing as the semaphore implementation.
            block_sc();
        }

        HOT NORETURN
        static void ret_user_sysexit();

        HOT NORETURN
        static void ret_user_iret() asm ("ret_user_iret");

        NORETURN
        static void ret_user_vmresume();

        NORETURN
        static void ret_user_vmrun();

        template <Sys_regs::Status T>
        NOINLINE NORETURN
        static void sys_finish();

        NORETURN
        void activate();

        template <void (*)()>
        NORETURN
        static void send_msg();

        HOT NORETURN
        static void recv_kern();

        HOT NORETURN
        static void recv_user();

        HOT NORETURN
        static void reply (void (*)() = nullptr);

        HOT NORETURN
        static void sys_call();

        HOT NORETURN
        static void sys_reply();

        NORETURN
        static void sys_create_pd();

        NORETURN
        static void sys_create_ec();

        NORETURN
        static void sys_create_sc();

        NORETURN
        static void sys_create_pt();

        NORETURN
        static void sys_create_sm();

        NORETURN
        static void sys_create_vi();

        NORETURN
        static void sys_revoke();

        NORETURN
        static void sys_lookup();

        NORETURN
        static void sys_ec_ctrl();

        NORETURN
        static void sys_sc_ctrl();

        NORETURN
        static void sys_pt_ctrl();

        NORETURN
        static void sys_sm_ctrl();

        NORETURN
        static void sys_vi_ctrl();

        NORETURN
        static void sys_assign_pci();

        NORETURN
        static void sys_assign_gsi();

        NORETURN
        static void idle();

        NORETURN
        static void root_invoke();

        template <bool>
        NORETURN
        static void delegate();

        NORETURN
        static void dead() { die ("IPC Abort"); }

        NORETURN
        static void die (char const *, Exc_regs * = &current->regs);

        ALWAYS_INLINE
        static inline void *operator new (size_t) { return cache.alloc(); }

        ALWAYS_INLINE
        static inline void operator delete (void *ptr) { cache.free (ptr); }
};
