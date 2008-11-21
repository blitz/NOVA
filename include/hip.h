/*
 * Hypervisor Information Page (HIP)
 *
 * Copyright (C) 2008, Udo Steinberg <udo@hypervisor.org>
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
#include "config.h"
#include "types.h"

class Hip_cpu
{
    public:
        uint8   flags;
        uint8   thread;
        uint8   core;
        uint8   package;
        uint32  reserved;
};

class Hip_mem
{
    public:
        enum {
            HYPERVISOR  = -1ul,
            MB_MODULE   = -2ul
        };

        uint64  addr;
        uint64  size;
        uint32  type;
        uint32  aux;
};

class Hip
{
    private:
        uint32  signature;              // 0x0
        uint16  checksum;               // 0x4
        uint16  length;                 // 0x6
        uint16  cpu_offs;               // 0x8
        uint16  cpu_size;               // 0xa
        uint16  mem_offs;               // 0xc
        uint16  mem_size;               // 0xe
        uint32  api_flg;                // 0x10
        uint32  api_ver;                // 0x14
        uint32  cfg_cap;                // 0x18
        uint32  cfg_pre;                // 0x1c
        uint32  cfg_gsi;                // 0x20
        uint32  reserved;               // 0x24
        uint32  cfg_page;               // 0x28
        uint32  cfg_utcb;               // 0x2c
        Hip_cpu cpu_desc[NUM_CPU];
        Hip_mem mem_desc[];

    public:
        static mword root_addr;
        static mword root_size;

        INIT
        static void build (mword);

        INIT
        static void add_mem (Hip_mem *&, mword, size_t);

        INIT
        static void add_mod (Hip_mem *&, mword, size_t);

        INIT
        static void add_mhv (Hip_mem *&);

        static void add_cpu();

        static void add_checksum();
};
