/*
 * Keyboard
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

#include "acpi.h"
#include "cmdline.h"
#include "gsi.h"
#include "keyb.h"
#include "stdio.h"

unsigned Keyb::gsi;

/*
 * Keyboard Initialization
 */
void Keyb::init()
{
    if (Cmdline::nokeyb) {
        gsi = ~0u;
        return;
    }

    send_ctrl (CMD_RD_CCB);
    unsigned ccb = read_keyb();

    send_keyb (KEYB_READ_ID);
    unsigned id = read_keyb() == KEYB_ACK ? read_keyb() | read_keyb() << 8 : 0;

    gsi = Gsi::irq_to_gsi (irq);
    trace (TRACE_KEYB, "KEYB: GSI:%#x CCB:%#x ID:%#x", gsi, ccb, id);
    Gsi::unmask (gsi);
}

void Keyb::interrupt()
{
    unsigned sts;

    while ((sts = status()) & STS_OUTB) {

        unsigned inp = input();

        if (sts & STS_AUXB)
            return;

        if (inp & 0x80)
            return;

        switch (inp) {
            case 0x1:
                Acpi::reset();
                send_ctrl (CMD_RESET);
                Io::out<uint8>(0xcf9, 0x6);
                return;
            case 0x3b ... 0x42:
                screen.set_page (inp - 0x3b);
                return;
        }
    }
}
