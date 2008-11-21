/*
 * Utility Functions
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

template <typename T>
ALWAYS_INLINE
static inline T min (T v1, T v2)
{
    return v1 < v2 ? v1 : v2;
}

template <typename T>
ALWAYS_INLINE
static inline T max (T v1, T v2)
{
    return v1 > v2 ? v1 : v2;
}
