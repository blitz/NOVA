/*
 * Protection Domain
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

#include "lock_guard.h"
#include "mdb.h"
#include "pd.h"
#include "pt.h"
#include "stdio.h"
#include "util.h"
#include "wq.h"

// PD Cache
Slab_cache Pd::cache ("PD", sizeof (Pd), 8);

// Current PD
Pd *Pd::current;

// Kernel PD
Pd Pd::kern (Ptab::master());

// Root PD
Pd *Pd::root;

Pd::Pd() : Space_io (this), Space_obj (this), ref (1)
{
    trace (TRACE_SYSCALL, "PD:%p created", this);
}

void Pd::delegate_mem (mword snd_base, mword rcv_base, size_t size, unsigned attr)
{
    if (current == root) {
        for (size_t i = 0; i < size; i += PAGE_SIZE) {
            Mr *mr = Space_mem::array + ((snd_base + i) >> PAGE_BITS);
            Lock_guard <Spinlock> guard (mr->lock);
            if (Space_mem::insert (rcv_base + i, snd_base + i, PAGE_SIZE, Ptab::Attribute (Ptab::ATTR_USER | Ptab::ATTR_WRITABLE)))
                mr->node = new Mn (current, rcv_base + i, attr);
        }
        return;
    }

    for (size_t i = 0; i < size; i += PAGE_SIZE) {

        Paddr phys;
        if (!current->Space_mem::lookup (snd_base + i, phys))
            continue;

        Mr *mr = Space_mem::array + (phys >> PAGE_BITS);
        Lock_guard <Spinlock> guard (mr->lock);

        Mn *node = mr->node->lookup (current, snd_base + i);
        if (!node)
            continue;

        attr &= node->attr;
        Ptab::Attribute a = Ptab::Attribute (Ptab::ATTR_USER |
                               (attr & 0x4 ? Ptab::ATTR_NONE     : Ptab::ATTR_NOEXEC) |
                               (attr & 0x2 ? Ptab::ATTR_WRITABLE : Ptab::ATTR_NONE));

        if (Space_mem::insert (rcv_base + i, phys, PAGE_SIZE, a))
            node->create_child (this, rcv_base + i, attr);
    }
}

void Pd::delegate_io (mword snd_base, size_t size)
{
    if (current == root) {
        for (size_t i = 0; i < size; i++) {
            Mr *mr = Space_io::array + snd_base + i;
            Lock_guard <Spinlock> guard (mr->lock);
            if (Space_io::insert (snd_base + i))
                mr->node = new Mn (current, snd_base + i, 0);
        }
        return;
    }

    for (size_t i = 0; i < size; i++) {

        if (!Space_io::lookup (snd_base + i))
            continue;

        Mr *mr = Space_io::array + snd_base + i;
        Lock_guard <Spinlock> guard (mr->lock);

        Mn *node = mr->node->lookup (current, snd_base + i);
        if (!node)
            continue;

        if (Space_io::insert (snd_base + i))
            node->create_child (this, snd_base + i, 0);
    }
}

void Pd::delegate_obj (mword snd_base, mword rcv_base, size_t size)
{
    for (size_t i = 0; i < size; i++) {

        Capability cap = Space_obj::lookup (snd_base + i);
        if (cap.type() != Capability::PT)
            continue;

        Pt *pt = cap.obj<Pt>();
        Lock_guard <Spinlock> guard (pt->lock);

        Mn *node = pt->node.lookup (current, snd_base + i);
        if (!node)
            continue;

        if (Space_obj::insert (rcv_base + i, cap))
            node->create_child (this, rcv_base + i, 0);
    }
}

void Pd::revoke_mem (mword base, size_t size, bool self)
{
    for (size_t i = 0; i < size; i += PAGE_SIZE) {

        Paddr phys;
        if (!current->Space_mem::lookup (base + i, phys))
            continue;

        Mr *mr = Space_mem::array + (phys >> PAGE_BITS);
        Lock_guard <Spinlock> guard (mr->lock);

        Mn *node = mr->node->lookup (current, base + i);
        if (!node)
            continue;

        for (Mn *chld; (chld = node->remove_child()); delete chld)
            chld->pd->Space_mem::remove (chld->cd, PAGE_SIZE);

        if (!self || !current->Space_mem::remove (base + i, PAGE_SIZE) || !node->root())
            continue;

        delete mr->node; mr->node = 0;
    }
}

void Pd::revoke_io (mword base, size_t size, bool self)
{
    for (size_t i = 0; i < size; i++) {

        if (!Space_io::lookup (base + i))
            continue;

        Mr *mr = Space_io::array + base + i;
        Lock_guard <Spinlock> guard (mr->lock);

        Mn *node = mr->node->lookup (current, base + i);
        if (!node)
            continue;

        for (Mn *chld; (chld = node->remove_child()); delete chld)
            chld->pd->Space_io::remove (chld->cd);

        if (!self || !current->Space_io::remove (base + i) || !node->root())
            continue;

        delete mr->node; mr->node = 0;
    }
}

void Pd::revoke_obj (mword base, size_t size, bool self)
{
    for (size_t i = 0; i < size; i++) {

        Capability cap = Space_obj::lookup (base + i);

        switch (cap.type()) {

            case Capability::PT:
                revoke_pt (cap, base + i, self);
                break;

            /*
             * After removing the root capability of a kernel object
             * new references cannot be taken. Wait one RCU epoch and
             * then decrement refcount. If 0, deallocate.
             */

            case Capability::WQ:
                if (current->Space_obj::remove (base + i, cap)) {
                    Wq *wq = cap.obj<Wq>();
                    if (wq->del_ref())
                        delete wq;
                }
                break;

            case Capability::EC:
                if (current->Space_obj::remove (base + i, cap)) {
                    Ec *ec = cap.obj<Ec>();
                    if (ec->del_ref())
                        delete ec;
                }
                break;

            case Capability::SC:
                if (current->Space_obj::remove (base + i, cap))
                    delete cap.obj<Sc>();
                break;

            case Capability::PD:
                if (current->Space_obj::remove (base + i, cap)) {
                    Pd *pd = cap.obj<Pd>();
                    if (pd->del_ref())
                        delete pd;
                }
                break;

            default:
                break;
        }
    }
}

void Pd::revoke_pt (Capability cap, mword cd, bool self)
{
    Pt *pt = cap.obj<Pt>();
    Lock_guard <Spinlock> guard (pt->lock);

    Mn *node = pt->node.lookup (current, cd);
    if (!node)
        return;

    for (Mn *chld; (chld = node->remove_child()); delete chld)
        chld->pd->Space_obj::remove (chld->cd, cap);

    if (!self || !current->Space_obj::remove (cd, cap) || !node->root())
        return;

    delete pt;
}

size_t Pd::adj_hot_spot (mword &snd_base, mword &rcv_base, mword snd_ord, mword rcv_ord, mword h)
{
    mword s = snd_ord < sizeof (mword) * 8 ? (1ul << snd_ord) - 1 : ~0ul;
    mword r = rcv_ord < sizeof (mword) * 8 ? (1ul << rcv_ord) - 1 : ~0ul;

    snd_base &= ~s;
    rcv_base &= ~r;

    if (EXPECT_TRUE (s <= r)) {
        rcv_base |= h & r & ~s;
        return s + 1;
    } else {
        snd_base |= h & s & ~r;
        return r + 1;
    }
}

size_t Pd::adj_identity (mword &snd_base, mword rcv_base, mword snd_ord, mword rcv_ord)
{
    mword s = (1ul << snd_ord) - 1;
    mword r = (1ul << rcv_ord) - 1;

    if ((snd_base ^ rcv_base) & ~(s | r))
        return 0;

    snd_base |= rcv_base;

    return (s & r) + 1;
}

void Pd::delegate (Crd rcv, Crd snd, mword hot)
{
    if (rcv.type() != snd.type()) {
        trace (TRACE_MAP, "CRD mismatch SND:%u RCV:%u", snd.type(), rcv.type());
        return;
    }

    mword snd_base = snd.base();
    mword rcv_base = rcv.base();
    mword snd_ord  = snd.order();
    mword rcv_ord  = rcv.order();

    size_t size;

    switch (snd.type()) {

        case Crd::MEM:
            snd_base <<= 10;
            rcv_base <<= 10;
            size = adj_hot_spot (snd_base, rcv_base, snd_ord + 10, rcv_ord + 10, hot);
            trace (TRACE_MAP, "MAP MEM PD:%p->%p HOT:%#010lx SB:%#010lx RB:%#010lx C:%#x P:%#x", current, this, hot, snd_base, rcv_base, size, snd.attr());
            delegate_mem (snd_base, rcv_base, size, snd.attr());
            break;

        case Crd::IO:
            size = adj_identity (snd_base, rcv_base, snd_ord, rcv_ord);
            trace (TRACE_MAP, "MAP I/O PD:%p->%p HOT:%#010lx SB:%#010lx C:%#x", current, this, hot, snd_base, size);
            delegate_io (snd_base, size);
            break;

        case Crd::OBJ:
            size = adj_hot_spot (snd_base, rcv_base, snd_ord, rcv_ord, hot);
            trace (TRACE_MAP, "MAP OBJ PD:%p->%p HOT:%#010lx SB:%#010lx RB:%#010lx C:%#x", current, this, hot, snd_base, rcv_base, size);
            delegate_obj (snd_base, rcv_base, size);
            break;
    }
}

void Pd::revoke (Crd rev, bool self)
{
    mword base = rev.base();
    mword size = 1ul << rev.order();

    switch (rev.type()) {

        case Crd::MEM:
            base <<= 10;
            trace (TRACE_MAP, "UNMAP MEM PD:%p B:%#010lx S:%#lx", current, base, size);
            revoke_mem (base, size, self);
            break;

        case Crd::IO:
            trace (TRACE_MAP, "UNMAP I/O PD:%p B:%#010lx S:%#lx", current, base, size);
            revoke_io (base, size, self);
            break;

        case Crd::OBJ:
            trace (TRACE_MAP, "UNMAP OBJ PD:%p B:%#010lx S:%#lx", current, base, size);
            revoke_obj (base, size, self);
            break;
    }
}

void Pd::delegate_items (Crd rcv, mword *ptr, unsigned items)
{
    while (items--) {

        mword hot = *ptr++;
        Crd snd = Crd (*ptr++);

        delegate (rcv, snd, hot);
    }
}
