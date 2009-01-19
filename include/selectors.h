/*
 * Selectors
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

#define SEL_NULL_DESC   0x0
#define SEL_KERN_CODE   0x8
#define SEL_KERN_DATA   0x10
#define SEL_USER_CODE   0x1b
#define SEL_USER_DATA   0x23
#define SEL_TSS_RUN     0x28
#define SEL_TSS_DBF     0x30

#ifdef NIXON
#define SEL_TSS_NMI     0x38
#define SEL_TSS_DBG     0x40
#define SEL_MAX         0x48
#else
#define SEL_MAX         0x38
#endif
