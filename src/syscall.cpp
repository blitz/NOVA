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

#include "ec.h"
#include "gsi.h"
#include "mtd.h"
#include "pd.h"
#include "pt.h"
#include "sc.h"
#include "stdio.h"
#include "wq.h"
#include "utcb.h"

void Ec::sys_finish (Sys_regs *param, Sys_regs::Status status)
{
    param->set_status (status);

    ret_user_sysexit();
}

void Ec::recv_ipc_msg (mword ip, unsigned flags)
{
    Sys_ipc_recv *r = static_cast<Sys_ipc_recv *>(&regs);

    r->set_ip (ip);

    if (EXPECT_FALSE (flags & Sys_ipc_send::DISABLE_DONATION)) {
        release();
        Sc::schedule();
    }

    rp = Reply_capability (current, Sc::current);
    Sc::current->set_user (this);

    make_current();
}

void Ec::send_vmx_msg()
{
    Exc_regs *r = &current->regs;

    Capability cap = Space_obj::lookup (current->regs.dst_portal);
    if (EXPECT_FALSE (cap.type() != Capability::PT)) {
        trace (TRACE_MSG, "EC:%p PT:%02lu (VMX)", current, current->regs.dst_portal);
        current->kill (r, "VMX PT");
    }

    Pt *pt = cap.obj<Pt>();

    Ec *ec = pt->wq->wait_dequeue();
    if (EXPECT_FALSE (!ec))
        pt->wq->block (send_vmx_msg);

    current->continuation = ret_user_vmresume;
    ec->utcb->load (r, current->vmcs, pt->mtd);
    ec->utcb->pid = pt->id;
    ec->recv_ipc_msg (pt->ip);
}

void Ec::send_exc_msg()
{
    Exc_regs *r = &current->regs;

    Capability cap = Space_obj::lookup (current->regs.dst_portal);
    if (EXPECT_FALSE (cap.type() != Capability::PT)) {
        trace (TRACE_MSG, "EC:%p PT:%02lu (EXC)", current, current->regs.dst_portal);
        current->kill (r, "EXC PT");
    }

    Pt *pt = cap.obj<Pt>();

    Ec *ec = pt->wq->wait_dequeue();
    if (EXPECT_FALSE (!ec))
        pt->wq->block (send_exc_msg);

    current->continuation = ret_user_iret;
    ec->utcb->load (r, pt->mtd);
    ec->utcb->pid = pt->id;
    ec->recv_ipc_msg (pt->ip);
}

void Ec::sys_ipc_call()
{
    Sys_ipc_send *s = static_cast<Sys_ipc_send *>(&current->regs);

    Capability cap = Space_obj::lookup (s->pt());
    if (EXPECT_FALSE (cap.type() != Capability::PT))
        sys_finish (s, Sys_regs::BAD_CAP);

    Pt *pt = cap.obj<Pt>();

    Ec *ec = pt->wq->wait_dequeue();
    if (EXPECT_FALSE (!ec)) {
        if (EXPECT_FALSE (pt->ip >= LINK_ADDR))
            reinterpret_cast<void (*)(Pt *)>(pt->ip)(pt);
        if (EXPECT_FALSE (s->flags() & Sys_ipc_send::DISABLE_BLOCKING))
            sys_finish (s, Sys_regs::TIMEOUT);
        pt->wq->block (sys_ipc_call);
    }

    mword *item = current->utcb->save (ec->utcb, s->mtd().untyped());

    unsigned typed = s->mtd().typed();
    if (EXPECT_FALSE (typed))
        ec->pd->delegate_items (ec->utcb->crd, item, typed);

    current->continuation = ret_user_sysexit;

    ec->utcb->pid = pt->id;
    ec->recv_ipc_msg (pt->ip, s->flags());
}

void Ec::sys_ipc_reply()
{
    Sys_ipc_repl *s = static_cast<Sys_ipc_repl *>(&current->regs);

    Capability cap = Space_obj::lookup (s->wq());
    if (EXPECT_FALSE (cap.type() != Capability::WQ))
        sys_finish (s, Sys_regs::BAD_CAP);

    Wq *wq = cap.obj<Wq>();

    // XXX: revoke reply capability
    Ec *ec = current->rp.check_ec();
    Sc *sc = current->rp.check_sc();

    if (EXPECT_TRUE (ec)) {

        assert (sc == Sc::current);

        mword *item;

        if (EXPECT_TRUE (ec->continuation == ret_user_sysexit))
            item = current->utcb->save (ec->utcb, s->mtd().untyped());
        else if (ec->continuation == ret_user_iret)
            item = current->utcb->save (&ec->regs, s->mtd());
        else if (ec->continuation == ret_user_vmresume)
            item = current->utcb->save (&ec->regs, ec->vmcs, s->mtd());
        else
            UNREACHED;

        unsigned typed = s->mtd().typed();
        if (EXPECT_FALSE (typed)) {
            Crd crd = ec->continuation == ret_user_sysexit ? ec->utcb->crd : Crd (Crd::MEM, 0, Crd::whole);
            ec->pd->delegate_items (crd, item, typed);
        }

        // Terminate donation
        sc->set_user (ec);
    }

    wq->wait_enqueue (current);

    current->continuation = ret_user_sysexit;

    Ec *next = wq->send_dequeue();

    if (EXPECT_FALSE (next))
        next->release();
    else if (EXPECT_TRUE (ec))
        ec->make_current();

    Sc::schedule (!ec);
}

void Ec::sys_ipc_gsi (Pt *pt)
{
    Sys_ipc_send *s = static_cast<Sys_ipc_send *>(&current->regs);

    if (s->mtd().xfer (Mtd::UNMASK))
        Gsi::unmask (pt->id);

    if (s->mtd().xfer (Mtd::WAIT)) {
        s->set_status (Sys_regs::SUCCESS);
        pt->wq->block (ret_user_sysexit);
    }

    sys_finish (s, Sys_regs::SUCCESS);
}

void Ec::sys_create_pt()
{
    Sys_create_pt *r = static_cast<Sys_create_pt *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PT:%02u WQ:%02u EIP:%#lx", current, r->pt(), r->wq(), r->eip());

    if (EXPECT_FALSE (r->eip() >= LINK_ADDR)) {
        trace (TRACE_SYSCALL, "Invalid EIP");
        sys_finish (r, Sys_regs::BAD_MEM);
    }

    Capability cap = Space_obj::lookup (r->wq());
    if (EXPECT_FALSE (cap.type() != Capability::WQ)) {
        trace (TRACE_SYSCALL, "Non-WQ CAP (%u)", Space_obj::lookup (r->wq()).type());
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    Wq *wq = cap.obj<Wq>();

    Pt *pt = new Pt (wq, r->mtd(), r->pt(), r->eip());
    if (!Pd::current->Space_obj::insert (r->pt(), Capability (pt))) {
        trace (TRACE_SYSCALL, "Non-NULL CAP (%u)", Space_obj::lookup (r->pt()).type());
        delete pt;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_create_wq()
{
    Sys_create_wq *r = static_cast<Sys_create_wq *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE WQ:%02u", current, r->wq());

    Wq *wq = new Wq;
    if (!Pd::current->Space_obj::insert (r->wq(), Capability (wq))) {
        trace (TRACE_SYSCALL, "Non-NULL CAP (%u)", Space_obj::lookup (r->wq()).type());
        delete wq;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_create_ec()
{
    Sys_create_ec *r = static_cast<Sys_create_ec *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE EC:%02u WQ:%02u UTCB:%#lx ESP:%#lx", current, r->ec(), r->wq(), r->utcb(), r->esp());

    if (EXPECT_FALSE (r->utcb() >= LINK_ADDR || r->utcb() & PAGE_MASK)) {
        trace (TRACE_SYSCALL, "Invalid UTCB address");
        sys_finish (r, Sys_regs::BAD_MEM);
    }

    Capability cap = Space_obj::lookup (r->wq());
    if (EXPECT_FALSE (cap.type() != Capability::WQ)) {
        trace (TRACE_SYSCALL, "Non-WQ CAP (%u)", Space_obj::lookup (r->wq()).type());
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    Wq *wq = cap.obj<Wq>();

    Ec *ec = new Ec (Pd::current, ret_user_sysexit, r->utcb());
    if (!Pd::current->Space_obj::insert (r->ec(), Capability (ec))) {
        trace (TRACE_SYSCALL, "Non-NULL CAP (%u)", Space_obj::lookup (r->ec()).type());
        delete ec;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    static_cast<Sys_ipc_recv *>(&ec->regs)->set_sp (r->esp());

    wq->wait_enqueue (ec);

    // XXX: If next has a higher priority than current, preemption must occur
    Ec *next = wq->send_dequeue();
    if (next)
        next->release();

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_create_sc()
{
    Sys_create_sc *r = static_cast<Sys_create_sc *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE SC:%02u EC:%02u P:%#lx Q:%#lx", current, r->sc(), r->ec(), r->prio(), r->quantum());

    Capability cap = Space_obj::lookup (r->ec());
    if (EXPECT_FALSE (cap.type() != Capability::EC)) {
        trace (TRACE_SYSCALL, "Non-EC CAP (%u)", Space_obj::lookup (r->ec()).type());
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    Ec *ec = cap.obj<Ec>();

    if (ec->sc)
        sys_finish (r, Sys_regs::BAD_CAP);

    Sc *sc = new Sc (ec, r->prio(), r->quantum());
    if (!Pd::current->Space_obj::insert (r->sc(), Capability (sc))) {
        trace (TRACE_SYSCALL, "Non-NULL CAP (%u)", Space_obj::lookup (r->sc()).type());
        delete sc;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_create_pd()
{
    Sys_create_pd *r = static_cast<Sys_create_pd *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_CREATE PD:%02u UTCB:%#lx", current, r->pd(), r->utcb());

    if (EXPECT_FALSE (r->utcb() >= LINK_ADDR || r->utcb() & PAGE_MASK)) {
        trace (TRACE_SYSCALL, "Invalid UTCB address");
        sys_finish (r, Sys_regs::BAD_MEM);
    }

    Pd *pd = new Pd;
    if (!Pd::current->Space_obj::insert (r->pd(), Capability (pd))) {
        trace (TRACE_SYSCALL, "Non-NULL CAP (%u)", Space_obj::lookup (r->pd()).type());
        delete pd;
        sys_finish (r, Sys_regs::BAD_CAP);
    }

    Ec *ec = new Ec (pd, r->utcb() ? ret_user_iret : ret_user_vmresume, r->utcb());
    Sc *sc = new Sc (ec, Sc::default_prio, Sc::default_quantum);
    sc->ready_enqueue();

    pd->Space_obj::insert (NUM_PRE + 0, Capability (ec));
    pd->Space_obj::insert (NUM_PRE + 1, Capability (sc));

    pd->delegate (Crd (Crd::OBJ, 0, Crd::whole), r->crd());

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::sys_revoke()
{
    Sys_revoke *r = static_cast<Sys_revoke *>(&current->regs);

    trace (TRACE_SYSCALL, "EC:%p SYS_REVOKE", current);

    Pd::revoke (r->crd(), r->self());

    sys_finish (r, Sys_regs::SUCCESS);
}

void Ec::syscall_handler (uint8 number)
{
    // This is actually faster than a switch
    if (EXPECT_TRUE (number == Sys_regs::MSG_CALL))
        sys_ipc_call();
    if (EXPECT_TRUE (number == Sys_regs::MSG_REPLY))
        sys_ipc_reply();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_PT))
        sys_create_pt();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_WQ))
        sys_create_wq();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_EC))
        sys_create_ec();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_SC))
        sys_create_sc();
    if (EXPECT_TRUE (number == Sys_regs::CREATE_PD))
        sys_create_pd();
    if (EXPECT_TRUE (number == Sys_regs::REVOKE))
        sys_revoke();

    sys_finish (&current->regs, Sys_regs::BAD_SYS);
}
