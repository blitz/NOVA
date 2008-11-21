/*
 * I/O Advanced Programmable Interrupt Controller (I/O APIC)
 *
 * Copyright (C) 2006-2008, Udo Steinberg <udo@hypervisor.org>
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

#include "ioapic.h"
#include "stdio.h"

Ioapic::Ioapic()
{
    trace (TRACE_APIC, "I/O APIC: LIN:%p ID:%#x VER:%#x IRT:%#x PRQ:%u",
           this, id(), version(), irt_max(), prq());
}
