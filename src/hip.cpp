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

#include "cmdline.h"
#include "cpu.h"
#include "extern.h"
#include "hip.h"
#include "multiboot.h"
#include "ptab.h"
#include "space_obj.h"

mword Hip::root_addr;
mword Hip::root_size;

void Hip::build (mword addr)
{
    Hip *hip = reinterpret_cast<Hip *>(&PAGE_H);

    hip->signature  = 0x41564f4e;
    hip->api_flg    = 0;
    hip->api_ver    = 0x1;
    hip->cfg_cap    = Space_obj::max;
    hip->cfg_pre    = NUM_PRE;
    hip->cfg_gsi    = NUM_GSI;
    hip->cfg_page   = 0;
    hip->cfg_utcb   = 0;
    hip->cpu_offs   = reinterpret_cast<mword>(hip->cpu_desc) - reinterpret_cast<mword>(hip);
    hip->cpu_size   = sizeof (Hip_cpu);
    hip->mem_offs   = reinterpret_cast<mword>(hip->mem_desc) - reinterpret_cast<mword>(hip);
    hip->mem_size   = sizeof (Hip_mem);

    Multiboot *mbi = static_cast<Multiboot *>(Ptab::master()->remap (addr));

    uint32 flags       = mbi->flags;
    uint32 mmap_addr   = mbi->mmap_addr;
    uint32 mmap_len    = mbi->mmap_len;
    uint32 mods_addr   = mbi->mods_addr;
    uint32 mods_count  = mbi->mods_count;

    // Parse our own command line.
    if (mbi->flags & Multiboot::CMDLINE) {
      char cmdline_buf[128];
      char *mbi_cmdline = static_cast<char *>(Ptab::master()->remap(mbi->cmdline));

      strncpy(cmdline_buf, mbi_cmdline, sizeof (cmdline_buf));
      Cmdline::init(cmdline_buf);
    }

    Hip_mem *mem = hip->mem_desc;

    if (flags & Multiboot::MEMORY_MAP)
        add_mem (mem, mmap_addr, mmap_len);

    if (flags & Multiboot::MODULES)
        add_mod (mem, mods_addr, mods_count);

    add_mhv (mem);

    hip->length = static_cast<uint16>(reinterpret_cast<mword>(mem) - reinterpret_cast<mword>(hip));
}

void Hip::add_mem (Hip_mem *&mem, mword addr, size_t len)
{
    char *mmap_addr = static_cast<char *>(Ptab::master()->remap (addr));

    for (char *ptr = mmap_addr; ptr < mmap_addr + len; mem++) {

        Multiboot_mmap *map = reinterpret_cast<Multiboot_mmap *>(ptr);

        mem->addr = map->addr;
        mem->size = map->len;
        mem->type = map->type;
        mem->aux  = 0;

        ptr += map->size + 4;
    }
}

void Hip::add_mod (Hip_mem *&mem, mword addr, size_t count)
{
    Multiboot_module *mod = static_cast<Multiboot_module *>(Ptab::master()->remap (addr));

    if (count) {
        root_addr = mod->s_addr;
        root_size = mod->e_addr - mod->s_addr;
    }

    for (unsigned i = 0; i < count; i++, mod++, mem++) {

        mem->addr = mod->s_addr;
        mem->size = mod->e_addr - mod->s_addr;
        mem->type = Hip_mem::MB_MODULE;
        mem->aux  = mod->cmdline;
    }
}

void Hip::add_mhv (Hip_mem *&mem)
{
    mem->addr = reinterpret_cast<mword>(&LINK_PHYS);
    mem->size = reinterpret_cast<mword>(&LINK_SIZE);
    mem->type = Hip_mem::HYPERVISOR;
    mem++;
}

void Hip::add_cpu()
{
    Hip *hip = reinterpret_cast<Hip *>(&PAGE_H);

    Hip_cpu *cpu = hip->cpu_desc + Cpu::id;
    cpu->package = static_cast<uint8>(Cpu::package);
    cpu->core    = static_cast<uint8>(Cpu::core);
    cpu->thread  = static_cast<uint8>(Cpu::thread);
    cpu->flags   = 1;
}

void Hip::add_checksum()
{
    Hip *hip = reinterpret_cast<Hip *>(&PAGE_H);

    uint16 c = 0;
    for (uint16 const *ptr = reinterpret_cast<uint16 const *>(&PAGE_H);
                       ptr < reinterpret_cast<uint16 const *>(&PAGE_H + hip->length);
                       c = static_cast<uint16>(c - *ptr++)) ;

    hip->checksum = c;
}
