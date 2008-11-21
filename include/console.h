/*
 * Generic Console
 *
 * Copyright (C) 2005-2008, Udo Steinberg <udo@hypervisor.org>
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

#include <stdarg.h>
#include "compiler.h"
#include "types.h"

class Console
{
    private:
        virtual void putc (int c) = 0;

        void print_number (uint64 val, unsigned base, unsigned width, unsigned flags);

        inline void print_str (char const *str, unsigned width, unsigned precs);

        FORMAT (2,0)
        inline char const *print_format (char const *format, va_list& args);

    protected:
        bool initialized;

    public:
        Console() : initialized (false) {}

        FORMAT (2,0)
        void vprintf (char const *format, va_list args);
};
