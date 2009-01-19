/*
 * String Functions
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

extern "C" ALWAYS_INLINE NONNULL
inline void *memcpy (void *d, void const *s, size_t n)
{
    mword dummy;
    asm volatile ("rep; movsb"
                  : "=D" (dummy), "+S" (s), "+c" (n)
                  : "0" (d)
                  : "memory");
    return d;
}

extern "C" ALWAYS_INLINE NONNULL
inline void *memset (void *d, int c, size_t n)
{
    mword dummy;
    asm volatile ("rep; stosb"
                  : "=D" (dummy), "+c" (n)
                  : "0" (d), "a" (c)
                  : "memory");
    return d;
}

extern "C" ALWAYS_INLINE NONNULL
inline int strcmp (char const *s1, char const *s2)
{
    while (*s1 && *s1 == *s2)
        s1++, s2++;

    return *s1 - *s2;
}

extern "C" ALWAYS_INLINE NONNULL
inline char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    char *di;

    for (i = 0, di = dest; i<n; i++) {
        di[0] = src[0];
        if (src[0] == 0)
            break;

        di++;
        src++;
    }

    return dest;
}
