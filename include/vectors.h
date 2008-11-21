/*
 * Interrupt Vectors
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

#pragma once

#include "config.h"

#define VEC_GSI         (NUM_EXC)
#define VEC_LVT         (NUM_EXC + NUM_GSI)
#define VEC_LVT_TIMER   (VEC_LVT + 0)
#define VEC_LVT_ERROR   (VEC_LVT + 3)
#define VEC_LVT_PERFM   (VEC_LVT + 4)
#define VEC_LVT_THERM   (VEC_LVT + 5)
#define VEC_MAX         (VEC_LVT + NUM_LVT)
