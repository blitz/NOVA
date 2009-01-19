/*
 * Virtual-Memory Layout
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

#define PAGE_BITS       12
#define PAGE_SIZE       (1 << PAGE_BITS)
#define PAGE_MASK       (PAGE_SIZE - 1)

#define LOAD_ADDR       0x200000
#define LINK_ADDR       0xc0000000

/*
 * Global Range from 0xc0000000 to 0xcfc00000
 */

/* HW Devices */
#define HWDEV_EADDR     0xcfbfe000

/* VGA Console */
#define VGACN_SADDR     0xcfbff000
#define VGACN_EADDR     VGACN_SADDR + PAGE_SIZE

/*
 * CPU Local Range from 0xcfe00000 to 0xd0000000
 */
#ifdef PAE
#define LOCAL_SADDR     0xcfe00000
#else
#define LOCAL_SADDR     0xcfc00000
#endif
#define LOCAL_EADDR     0xd0000000

#ifdef NIXON
/* NMI Stack */
#define NMISTCK_SADDR   0xcfffb000
#define NMISTCK_EADDR   NMISTCK_SADDR + PAGE_SIZE
#endif

/* Local APIC */
#define LAPIC_SADDR     0xcfffc000
#define LAPIC_EADDR     LAPIC_SADDR + PAGE_SIZE

/* Kernel Stack */
#define KSTCK_SADDR     0xcfffe000
#define KSTCK_EADDR     KSTCK_SADDR + PAGE_SIZE

/* CPU-Local */
#define CPULC_SADDR     0xcffff000
#define CPULC_EADDR     CPULC_SADDR + PAGE_SIZE

/*
 * AS Local Range from 0xd0000000 to max
 */

/* I/O Space */
#define IOBMP_SADDR     0xd0000000
#define IOBMP_EADDR     IOBMP_SADDR + PAGE_SIZE * 2

/* Remap Window */
#define REMAP_SADDR     0xdf000000
#define REMAP_EADDR     0xdf800000

/* Object Space */
#define OBJSP_SADDR     0xe0000000
#define OBJSP_EADDR     OBJSP_SADDR + 0x20000000
