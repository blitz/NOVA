/*
 * Atomic Operations
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

#define LOCK_UP ""
#define LOCK_MP "lock;"

class Atomic
{
    public:
        template <typename T>
        ALWAYS_INLINE
        static inline void add (T &val, unsigned n)
        {
            asm volatile (LOCK_MP "add %1, %0" : "+m" (val) : "ir" (n) : "cc");
        }

        template <typename T>
        ALWAYS_INLINE
        static inline bool sub (T &val, unsigned n)
        {
            bool ret;
            asm volatile (LOCK_MP "sub %2, %1; setz %0" : "=q" (ret), "+m" (val) : "ir" (n) : "cc");
            return ret;
        }

        template <typename T>
        ALWAYS_INLINE
        static inline bool test_clr_bit_mp (T &val, unsigned bit)
        {
            bool ret;
            asm volatile (LOCK_MP "btr %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }

        template <typename T>
        ALWAYS_INLINE
        static inline bool test_set_bit_mp (T &val, unsigned bit)
        {
            bool ret;
            asm volatile (LOCK_MP "bts %2, %1; setc %0" : "=q" (ret), "+m" (val) : "ir" (bit) : "cc");
            return ret;
        }

        template <typename T>
        ALWAYS_INLINE
        static inline void set_mask_mp (T &val, unsigned mask)
        {
            asm volatile (LOCK_MP "or %1, %0" : "+m" (val) : "ir" (mask) : "cc");
        }

        template <typename T>
        ALWAYS_INLINE
        static inline void clr_mask_mp (T &val, unsigned mask)
        {
            asm volatile (LOCK_MP "and %1, %0" : "+m" (val) : "ir" (~mask) : "cc");
        }

        template <typename T>
        ALWAYS_INLINE
        static inline bool cmp_swap_up (T *ptr, T o, T n)
        {
            bool ret;
            asm volatile (LOCK_UP "cmpxchg %3, %1; sete %0" : "=q" (ret), "+m" (*ptr), "+a" (o) : "r" (n) : "cc");
            return ret;
        }

        template <typename T>
        ALWAYS_INLINE
        static inline bool cmp_swap_mp (T *ptr, T o, T n)
        {
            bool ret;
            asm volatile (LOCK_MP "cmpxchg %3, %1; sete %0" : "=q" (ret), "+m" (*ptr), "+a" (o) : "r" (n) : "cc");
            return ret;
        }

        template <typename T>
        ALWAYS_INLINE
        static inline T exchange (T *ptr, T n)
        {
            asm volatile ("xchg %1, %0" : "+m" (*ptr), "+r" (n) : : "cc");
            return n;
        }
};
