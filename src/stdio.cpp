/*
 * Standard I/O
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

#include "initprio.h"
#include "spinlock.h"
#include "stdio.h"

INIT_PRIORITY (PRIO_CONSOLE)
Console_serial serial;

INIT_PRIORITY (PRIO_CONSOLE)
Console_vga screen;

INIT_PRIORITY (PRIO_CONSOLE)
Spinlock printf_lock;

unsigned init_spinner (unsigned cpu)
{
    printf_lock.lock();
    unsigned val = screen.init_spinner (cpu);
    printf_lock.unlock();
    return val;
}

void vprintf (char const *format, va_list args)
{
    printf_lock.lock();
    screen.vprintf (format, args);
    serial.vprintf (format, args);
    printf_lock.unlock();
}

void panic (char const *format, ...)
{
    va_list args;
    va_start (args, format);
    screen.vprintf (format, args);      // Do not grab lock
    va_end (args);

    Cpu::shutdown();
}

void printf (char const *format, ...)
{
    va_list args;
    va_start (args, format);
    vprintf (format, args);
    va_end (args);
}
